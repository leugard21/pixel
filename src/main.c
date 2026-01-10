#include <SDL2/SDL.h>
#include <stdio.h>

#include "framebuffer.h"

static int sdl_fail(const char *msg) {
  fprintf(stderr, "%s: %s\n", msg, SDL_GetError());
  return 1;
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  if (SDL_Init(SDL_INIT_VIDEO) != 0)
    return sdl_fail("SDL_Init failed");

  const int width = 960;
  const int height = 540;

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

  int running = 1;
  while (running) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT)
        running = 0;
      if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
        running = 0;
    }

    fb_clear(&fb, 0xFF121212);

    fb_put_pixel(&fb, width / 2, height / 2, 0xFF0000);

    SDL_UpdateTexture(texture, 0, fb.pixels, width * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, 0, 0);
    SDL_RenderPresent(renderer);
  }

  fb_destroy(&fb);
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
