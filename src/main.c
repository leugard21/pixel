#include <SDL2/SDL.h>
#include <stdio.h>
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

typedef struct {
  int drawing;
  int last_x;
  int last_y;
  int brush_radius;
  uint32_t brush_color;
} App;

static void clamp_int(int *v, int lo, int hi) {
  if (*v < lo)
    *v = lo;
  if (*v > hi)
    *v = hi;
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

static void undo_action(History *undo, History *redo, Framebuffer *fb) {
  uint32_t *current =
      (uint32_t *)malloc(sizeof(uint32_t) * fb->width * fb->height);
  if (!current)
    return;

  for (int i = 0; i < fb->width * fb->height; i++)
    current[i] = fb->pixels[i];

  if (history_pop(undo, fb)) {
    Framebuffer tmp = *fb;
    tmp.pixels = current;
    history_push(redo, &tmp);
  }

  free(current);
}

static void redo_action(History *undo, History *redo, Framebuffer *fb) {
  uint32_t *current =
      (uint32_t *)malloc(sizeof(uint32_t) * fb->width * fb->height);
  if (!current)
    return;

  for (int i = 0; i < fb->width * fb->height; i++)
    current[i] = fb->pixels[i];

  if (history_pop(redo, fb)) {
    Framebuffer tmp = *fb;
    tmp.pixels = current;
    history_push(undo, &tmp);
  }

  free(current);
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

        if ((mod & KMOD_CTRL) && key == SDLK_z) {
          undo_action(&undo, &redo, &fb);
        }
        if ((mod & KMOD_CTRL) && key == SDLK_y) {
          redo_action(&undo, &redo, &fb);
        }
      } break;

      case SDL_MOUSEBUTTONDOWN:
        if (e.button.button == SDL_BUTTON_LEFT) {
          history_push(&undo, &fb);
          history_clear(&redo);

          app.drawing = 1;
          app.last_x = e.button.x;
          app.last_y = e.button.y;
          brush_stamp_circle(&fb, app.last_x, app.last_y, app.brush_radius,
                             app.brush_color);
        }
        break;

      case SDL_MOUSEBUTTONUP:
        if (e.button.button == SDL_BUTTON_LEFT) {
          app.drawing = 0;
        }
        break;

      case SDL_MOUSEMOTION:
        if (app.drawing) {
          int x = e.motion.x;
          int y = e.motion.y;
          brush_stroke_circle(&fb, app.last_x, app.last_y, x, y,
                              app.brush_radius, app.brush_color);
          app.last_x = x;
          app.last_y = y;
        }
        break;
      }
    }

    SDL_UpdateTexture(texture, 0, fb.pixels, width * (int)sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, 0, 0);
    SDL_RenderPresent(renderer);
  }

  history_destroy(&undo);
  history_destroy(&redo);
  fb_destroy(&fb);
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
