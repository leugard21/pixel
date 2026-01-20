#include <SDL2/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <direct.h>
static int make_dir(const char *path) { return _mkdir(path); }
#else
#include <sys/stat.h>
#include <sys/types.h>
static int make_dir(const char *path) { return mkdir(path, 0755); }
#endif

#include "brush.h"
#include "export.h"
#include "framebuffer.h"
#include "history.h"
#include "ui.h"

static int sdl_fail(const char *msg) {
  fprintf(stderr, "%s: %s\n", msg, SDL_GetError());
  return 1;
}

typedef enum { TOOL_BRUSH = 0, TOOL_LINE, TOOL_RECT, TOOL_CIRCLE } Tool;

typedef struct {
  float zoom;
  float offset_x;
  float offset_y;
} View;

typedef struct {
  int drawing;
  int start_x;
  int start_y;
  int last_x;
  int last_y;

  Tool tool;
  int fill;

  int brush_radius;
  uint32_t brush_color;

  uint32_t *base_pixels;
  int base_w;
  int base_h;

  View view;
  int panning;
  int pan_using_left;
  int pan_last_x;
  int pan_last_y;

  int show_grid;

  int show_hud;
  UI ui;
  UIText hud_text;
  int hud_dirty;
} App;

static const char *tool_name(Tool t) {
  switch (t) {
  case TOOL_BRUSH:
    return "BRUSH";
  case TOOL_LINE:
    return "LINE";
  case TOOL_RECT:
    return "RECT";
  case TOOL_CIRCLE:
    return "CIRCLE";
  }
  return "UNKNOWN";
}

static uint32_t palette_color(int idx) {
  switch (idx) {
  case 1:
    return ARGB(255, 240, 240, 240); // white
  case 2:
    return ARGB(255, 20, 20, 20); // black
  case 3:
    return ARGB(255, 255, 80, 80); // red
  case 4:
    return ARGB(255, 80, 255, 80); // green
  case 5:
    return ARGB(255, 80, 80, 255); // blue
  case 6:
    return ARGB(255, 255, 255, 80); // yellow
  case 7:
    return ARGB(255, 255, 80, 255); // magenta
  case 8:
    return ARGB(255, 80, 255, 255); // cyan
  default:
    return ARGB(255, 240, 240, 240);
  }
}

static int view_screen_to_canvas(const View *v, int sx, int sy, int *out_x,
                                 int *out_y) {
  if (v->zoom <= 0.0001f)
    return 0;
  float cx = (sx - v->offset_x) / v->zoom;
  float cy = (sy - v->offset_y) / v->zoom;
  *out_x = (int)cx;
  *out_y = (int)cy;
  return 1;
}

static SDL_Rect view_canvas_to_screen_rect(const View *v, int canvas_w,
                                           int canvas_h) {
  SDL_Rect r;
  r.x = (int)v->offset_x;
  r.y = (int)v->offset_y;
  r.w = (int)(canvas_w * v->zoom);
  r.h = (int)(canvas_h * v->zoom);
  return r;
}

static void render_grid(SDL_Renderer *r, const View *v, int canvas_w,
                        int canvas_h) {
  if (v->zoom < 6.0f)
    return;

  SDL_Rect dst = view_canvas_to_screen_rect(v, canvas_w, canvas_h);

  int win_w = 0, win_h = 0;
  SDL_GetRendererOutputSize(r, &win_w, &win_h);

  int left = dst.x;
  int top = dst.y;
  int right = dst.x + dst.w;
  int bottom = dst.y + dst.h;

  if (right < 0 || bottom < 0 || left > win_w || top > win_h)
    return;

  if (left < 0)
    left = 0;
  if (top < 0)
    top = 0;
  if (right > win_w)
    right = win_w;
  if (bottom > win_h)
    bottom = win_h;

  SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(r, 255, 255, 255, 35);

  float step = v->zoom;

  float mod_x = fmodf(v->offset_x, step);
  float mod_y = fmodf(v->offset_y, step);
  if (mod_x < 0)
    mod_x += step;
  if (mod_y < 0)
    mod_y += step;

  float first_x = (float)left + (step - mod_x);
  float first_y = (float)top + (step - mod_y);

  for (float x = first_x; x <= (float)right; x += step) {
    int xi = (int)(x + 0.5f);
    SDL_RenderDrawLine(r, xi, top, xi, bottom);
  }

  for (float y = first_y; y <= (float)bottom; y += step) {
    int yi = (int)(y + 0.5f);
    SDL_RenderDrawLine(r, left, yi, right, yi);
  }
}

static void clamp_int(int *v, int lo, int hi) {
  if (*v < lo)
    *v = lo;
  if (*v > hi)
    *v = hi;
}

static int isqrt_int(int v) {
  if (v <= 0)
    return 0;
  int x = v;
  int y = (x + 1) / 2;
  while (y < x) {
    x = y;
    y = (x + v / x) / 2;
  }
  return x;
}

static void app_base_init(App *app, int w, int h) {
  app->base_w = w;
  app->base_h = h;
  app->base_pixels = (uint32_t *)malloc(sizeof(uint32_t) * w * h);
  if (app->base_pixels)
    memset(app->base_pixels, 0, sizeof(uint32_t) * w * h);
}

static void app_base_destroy(App *app) {
  free(app->base_pixels);
  app->base_pixels = NULL;
  app->base_w = 0;
  app->base_h = 0;
}

static void app_base_capture(const App *app, const Framebuffer *fb) {
  if (!app->base_pixels)
    return;
  memcpy(app->base_pixels, fb->pixels,
         sizeof(uint32_t) * fb->width * fb->height);
}

static void app_base_restore(const App *app, Framebuffer *fb) {
  if (!app->base_pixels)
    return;
  memcpy(fb->pixels, app->base_pixels,
         sizeof(uint32_t) * fb->width * fb->height);
}

static void draw_shape_preview(const App *app, Framebuffer *fb, int x, int y) {
  if (app->tool == TOOL_LINE) {
    fb_draw_line(fb, app->start_x, app->start_y, x, y, app->brush_color);
    return;
  }

  if (app->tool == TOOL_RECT) {
    if (app->fill)
      fb_fill_rect(fb, app->start_x, app->start_y, x, y, app->brush_color);
    else
      fb_draw_rect(fb, app->start_x, app->start_y, x, y, app->brush_color);
    return;
  }

  if (app->tool == TOOL_CIRCLE) {
    int dx = x - app->start_x;
    int dy = y - app->start_y;
    int r = isqrt_int(dx * dx + dy * dy);
    if (app->fill)
      fb_fill_circle(fb, app->start_x, app->start_y, r, app->brush_color);
    else
      fb_draw_circle(fb, app->start_x, app->start_y, r, app->brush_color);
    return;
  }
}

void save_canvas_bmp(const Framebuffer *fb) {
  (void)make_dir("exports");

  time_t t = time(NULL);
  struct tm tmv;
#ifdef _WIN32
  localtime_s(&tmv, &t);
#else
  tmv = *localtime(&t);
#endif

  char stamp[32];
  strftime(stamp, sizeof(stamp), "%Y%m%d_%H%M%S", &tmv);

  char path[128];
  snprintf(path, sizeof(path), "exports/pixel_%s.bmp", stamp);

  if (export_bmp(fb, path)) {
    printf("Saved: %s\n", path);
  } else {
    printf("Save failed: %s (%s)\n", path, SDL_GetError());
  }
}

static void hud_mark_dirty(App *app) { app->hud_dirty = 1; }

static void hud_rebuild_if_needed(App *app, SDL_Renderer *r) {
  if (!app->show_hud)
    return;
  if (!app->ui.font)
    return;
  if (!app->hud_dirty)
    return;

  ui_text_destroy(&app->hud_text);

  char line[256];
  snprintf(line, sizeof(line),
           "Tool:%s  Brush:%d  Fill:%s  Grid:%s  Zoom:%d%%  Color:%08X",
           tool_name(app->tool), app->brush_radius, app->fill ? "ON" : "OFF",
           app->show_grid ? "ON" : "OFF", (int)(app->view.zoom * 100.0f),
           (unsigned)app->brush_color);

  if (ui_text_make(&app->ui, r, &app->hud_text, line)) {
    app->hud_dirty = 0;
  }
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  if (SDL_Init(SDL_INIT_VIDEO) != 0)
    return sdl_fail("SDL_Init failed");

  const int width = 800;
  const int height = 600;

  SDL_Window *window =
      SDL_CreateWindow("Pixel", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                       width, height, SDL_WINDOW_SHOWN);
  if (!window) {
    SDL_Quit();
    return sdl_fail("SDL_CreateWindow failed");
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer) {
    SDL_DestroyWindow(window);
    SDL_Quit();
    return sdl_fail("SDL_CreateRenderer failed");
  }

  SDL_Texture *texture =
      SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                        SDL_TEXTUREACCESS_STREAMING, width, height);
  if (!texture) {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return sdl_fail("SDL_CreateTexture failed");
  }

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

  Framebuffer fb;
  if (!fb_init(&fb, width, height)) {
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }
  fb_clear(&fb, ARGB(255, 18, 18, 18));

  History undo, redo;
  if (!history_init(&undo, 32, width, height)) {
    fb_destroy(&fb);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }
  if (!history_init(&redo, 32, width, height)) {
    history_destroy(&undo);
    fb_destroy(&fb);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  App app;
  memset(&app, 0, sizeof(app));

  app.brush_radius = 6;
  app.brush_color = ARGB(255, 240, 240, 240);

  app.tool = TOOL_BRUSH;
  app.fill = 0;

  app.view.zoom = 1.0f;
  app.view.offset_x = 0.0f;
  app.view.offset_y = 0.0f;

  app.show_grid = 1;

  app.show_hud = 1;
  app.hud_text.tex = NULL;
  app.hud_dirty = 1;

  app_base_init(&app, width, height);
  if (!app.base_pixels) {
    history_destroy(&undo);
    history_destroy(&redo);
    fb_destroy(&fb);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  const char *font_path = "assets/font.ttf";
  if (!ui_init(&app.ui, font_path, 14)) {
    printf("HUD disabled, missing font: %s\n", font_path);
    app.show_hud = 0;
  }

  int running = 1;
  while (running) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      switch (e.type) {
      case SDL_QUIT:
        running = 0;
        break;

      case SDL_KEYDOWN: {
        SDL_Keycode key = e.key.keysym.sym;
        SDL_Keymod mod = e.key.keysym.mod;

        if (key == SDLK_ESCAPE)
          running = 0;

        if (key == SDLK_LEFTBRACKET) {
          app.brush_radius--;
          clamp_int(&app.brush_radius, 1, 64);
          hud_mark_dirty(&app);
        }
        if (key == SDLK_RIGHTBRACKET) {
          app.brush_radius++;
          clamp_int(&app.brush_radius, 1, 64);
          hud_mark_dirty(&app);
        }

        if (key == SDLK_c) {
          fb_clear(&fb, ARGB(255, 18, 18, 18));
          history_clear(&undo);
          history_clear(&redo);
        }

        if (key == SDLK_F1) {
          app.tool = TOOL_BRUSH;
          hud_mark_dirty(&app);
        }
        if (key == SDLK_F2) {
          app.tool = TOOL_LINE;
          hud_mark_dirty(&app);
        }
        if (key == SDLK_F3) {
          app.tool = TOOL_RECT;
          hud_mark_dirty(&app);
        }
        if (key == SDLK_F4) {
          app.tool = TOOL_CIRCLE;
          hud_mark_dirty(&app);
        }

        if (key == SDLK_f) {
          app.fill = !app.fill;
          hud_mark_dirty(&app);
        }

        if (key == SDLK_r) {
          app.view.zoom = 1.0f;
          app.view.offset_x = 0.0f;
          app.view.offset_y = 0.0f;
          hud_mark_dirty(&app);
        }

        if (key == SDLK_g) {
          app.show_grid = !app.show_grid;
          hud_mark_dirty(&app);
        }

        if (key == SDLK_h) {
          app.show_hud = !app.show_hud;
          hud_mark_dirty(&app);
        }

        if ((mod & KMOD_CTRL) && key == SDLK_z) {
          uint32_t *prev = history_peek_copy(&undo);
          if (prev) {
            history_push(&redo, &fb);
            history_pop(&undo, &fb);
            free(prev);
          }
        }

        if ((mod & KMOD_CTRL) && key == SDLK_y) {
          uint32_t *next = history_peek_copy(&redo);
          if (next) {
            history_push(&undo, &fb);
            history_pop(&redo, &fb);
            free(next);
          }
        }

        if ((mod & KMOD_CTRL) && key == SDLK_s) {
          save_canvas_bmp(&fb);
        }

        if (key >= SDLK_1 && key <= SDLK_8) {
          int idx = (int)(key - SDLK_0);
          app.brush_color = palette_color(idx);
          hud_mark_dirty(&app);
        }
      } break;

      case SDL_MOUSEBUTTONDOWN:
        if (e.button.button == SDL_BUTTON_MIDDLE ||
            (e.button.button == SDL_BUTTON_LEFT &&
             (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_SPACE]))) {
          app.panning = 1;
          app.pan_using_left = (e.button.button == SDL_BUTTON_LEFT) ? 1 : 0;
          app.pan_last_x = e.button.x;
          app.pan_last_y = e.button.y;
          break;
        }

        if (e.button.button == SDL_BUTTON_LEFT) {
          SDL_Keymod mod = SDL_GetModState();
          if (mod & KMOD_ALT) {
            int cx, cy;
            if (view_screen_to_canvas(&app.view, e.button.x, e.button.y, &cx,
                                      &cy)) {
              uint32_t c = fb_get_pixel(&fb, cx, cy, ARGB(255, 0, 0, 0));
              app.brush_color = c;
              hud_mark_dirty(&app);
            }
            break;
          }

          history_push(&undo, &fb);
          history_clear(&redo);

          int cx, cy;
          if (!view_screen_to_canvas(&app.view, e.button.x, e.button.y, &cx,
                                     &cy))
            break;

          app.drawing = 1;
          app.start_x = cx;
          app.start_y = cy;
          app.last_x = cx;
          app.last_y = cy;

          if (app.tool == TOOL_BRUSH) {
            brush_stamp_circle(&fb, app.last_x, app.last_y, app.brush_radius,
                               app.brush_color);
          } else {
            app_base_capture(&app, &fb);
            app_base_restore(&app, &fb);
            draw_shape_preview(&app, &fb, app.last_x, app.last_y);
          }
        }
        break;

      case SDL_MOUSEBUTTONUP:
        if (app.panning) {
          if (e.button.button == SDL_BUTTON_MIDDLE ||
              (e.button.button == SDL_BUTTON_LEFT && app.pan_using_left)) {
            app.panning = 0;
            app.pan_using_left = 0;
          }
          break;
        }

        if (e.button.button == SDL_BUTTON_LEFT) {
          if (app.drawing && app.tool != TOOL_BRUSH) {
            int cx, cy;
            app_base_restore(&app, &fb);
            if (view_screen_to_canvas(&app.view, e.button.x, e.button.y, &cx,
                                      &cy)) {
              draw_shape_preview(&app, &fb, cx, cy);
            }
          }
          app.drawing = 0;
        }
        break;

      case SDL_MOUSEMOTION:
        if (app.panning) {
          int dx = e.motion.x - app.pan_last_x;
          int dy = e.motion.y - app.pan_last_y;
          app.view.offset_x += (float)dx;
          app.view.offset_y += (float)dy;
          app.pan_last_x = e.motion.x;
          app.pan_last_y = e.motion.y;
          break;
        }

        if (app.drawing) {
          int x, y;
          if (!view_screen_to_canvas(&app.view, e.motion.x, e.motion.y, &x, &y))
            break;

          if (app.tool == TOOL_BRUSH) {
            brush_stroke_circle(&fb, app.last_x, app.last_y, x, y,
                                app.brush_radius, app.brush_color);
          } else {
            app_base_restore(&app, &fb);
            draw_shape_preview(&app, &fb, x, y);
          }

          app.last_x = x;
          app.last_y = y;
        }
        break;

      case SDL_MOUSEWHEEL: {
        int mx, my;
        SDL_GetMouseState(&mx, &my);

        float old = app.view.zoom;
        float factor = (e.wheel.y > 0) ? 1.1f : 0.9f;
        float next = old * factor;
        if (next < 0.25f)
          next = 0.25f;
        if (next > 20.0f)
          next = 20.0f;

        if (next != old) {
          float cx = (mx - app.view.offset_x) / old;
          float cy = (my - app.view.offset_y) / old;

          app.view.zoom = next;
          app.view.offset_x = mx - cx * next;
          app.view.offset_y = my - cy * next;

          hud_mark_dirty(&app);
        }
      } break;
      }
    }

    hud_rebuild_if_needed(&app, renderer);

    SDL_UpdateTexture(texture, 0, fb.pixels, width * (int)sizeof(uint32_t));
    SDL_RenderClear(renderer);

    SDL_Rect dst = view_canvas_to_screen_rect(&app.view, fb.width, fb.height);
    SDL_RenderCopy(renderer, texture, 0, &dst);

    if (app.show_grid)
      render_grid(renderer, &app.view, fb.width, fb.height);

    if (app.show_hud && app.hud_text.tex) {
      int panel_w = app.hud_text.w + 20 + 26;
      ui_draw_panel(renderer, 10, 10, panel_w, app.hud_text.h + 16);

      SDL_Rect sw = {20, 18, 16, 16};
      uint32_t c = app.brush_color;
      Uint8 a = (Uint8)((c >> 24) & 0xFF);
      Uint8 rr = (Uint8)((c >> 16) & 0xFF);
      Uint8 gg = (Uint8)((c >> 8) & 0xFF);
      Uint8 bb = (Uint8)(c & 0xFF);

      SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
      SDL_SetRenderDrawColor(renderer, rr, gg, bb, a);
      SDL_RenderFillRect(renderer, &sw);
      SDL_SetRenderDrawColor(renderer, 255, 255, 255, 180);
      SDL_RenderDrawRect(renderer, &sw);

      ui_draw_text(renderer, &app.hud_text, 20 + 22, 18);
    }

    SDL_RenderPresent(renderer);
  }

  ui_text_destroy(&app.hud_text);
  ui_destroy(&app.ui);

  history_destroy(&undo);
  history_destroy(&redo);
  app_base_destroy(&app);

  fb_destroy(&fb);
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
