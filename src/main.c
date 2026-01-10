#include <SDL2/SDL.h>
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
  int pan_last_x;
  int pan_last_y;
} App;

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

static void clamp_int(int *v, int lo, int hi) {
  if (*v < lo)
    *v = lo;
  if (*v > hi)
    *v = hi;
}

static int iabs(int v) { return v < 0 ? -v : v; }

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
  if (app->base_pixels) {
    memset(app->base_pixels, 0, sizeof(uint32_t) * w * h);
  }
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

static void save_canvas_bmp(const Framebuffer *fb) {
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

static void restore_from_snapshot(uint32_t *snapshot, Framebuffer *fb) {
  if (!snapshot)
    return;
  for (int i = 0; i < fb->width * fb->height; i++)
    fb->pixels[i] = snapshot[i];
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
  app.drawing = 0;
  app.last_x = 0;
  app.last_y = 0;
  app.brush_radius = 6;
  app.brush_color = ARGB(255, 240, 240, 240);

  app.tool = TOOL_BRUSH;
  app.fill = 0;

  app.start_x = 0;
  app.start_y = 0;

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

  app.view.zoom = 1.0f;
  app.view.offset_x = 0.0f;
  app.view.offset_y = 0.0f;
  app.panning = 0;
  app.pan_last_x = 0;
  app.pan_last_y = 0;

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
        }
        if (key == SDLK_RIGHTBRACKET) {
          app.brush_radius++;
          clamp_int(&app.brush_radius, 1, 64);
        }
        if (key == SDLK_c) {
          fb_clear(&fb, ARGB(255, 18, 18, 18));
          history_clear(&undo);
          history_clear(&redo);
        }
        if (key == SDLK_s) {
          save_canvas_bmp(&fb);
        }
        if (key == SDLK_1)
          app.tool = TOOL_BRUSH;
        if (key == SDLK_2)
          app.tool = TOOL_LINE;
        if (key == SDLK_3)
          app.tool = TOOL_RECT;
        if (key == SDLK_4)
          app.tool = TOOL_CIRCLE;

        if (key == SDLK_f)
          app.fill = !app.fill;

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

        if (key == SDLK_r) {
          app.view.zoom = 1.0f;
          app.view.offset_x = 0.0f;
          app.view.offset_y = 0.0f;
        }

      } break;

      case SDL_MOUSEBUTTONDOWN:
        if (e.button.button == SDL_BUTTON_MIDDLE ||
            (e.button.button == SDL_BUTTON_LEFT &&
             (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_SPACE]))) {
          app.panning = 1;
          app.pan_last_x = e.button.x;
          app.pan_last_y = e.button.y;
          break;
        }

        if (e.button.button == SDL_BUTTON_LEFT) {
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
        if (e.button.button == SDL_BUTTON_MIDDLE ||
            (e.button.button == SDL_BUTTON_LEFT && app.panning)) {
          app.panning = 0;
          break;
        }

        if (e.button.button == SDL_BUTTON_LEFT) {
          if (app.drawing && app.tool != TOOL_BRUSH) {
            app_base_restore(&app, &fb);
            draw_shape_preview(&app, &fb, e.button.x, e.button.y);
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
            int cx, cy;

            app_base_restore(&app, &fb);
            if (view_screen_to_canvas(&app.view, e.button.x, e.button.y, &cx,
                                      &cy)) {
              draw_shape_preview(&app, &fb, cx, cy);
            }
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
        }
      } break;
      }
    }

    SDL_UpdateTexture(texture, 0, fb.pixels, width * (int)sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_Rect dst = view_canvas_to_screen_rect(&app.view, fb.width, fb.height);
    SDL_RenderCopy(renderer, texture, 0, &dst);
    SDL_RenderPresent(renderer);
  }

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
