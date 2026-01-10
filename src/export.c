#include "export.h"

#include <SDL2/SDL.h>

int export_bmp(const Framebuffer *fb, const char *path) {
  if (!fb || !fb->pixels || fb->width <= 0 || fb->height <= 0)
    return 0;

  const int pitch = fb->width * (int)sizeof(uint32_t);

  SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormatFrom(
      (void *)fb->pixels, fb->width, fb->height, 32, pitch,
      SDL_PIXELFORMAT_ARGB8888);
  if (!surface)
    return 0;

  int ok = (SDL_SaveBMP(surface, path) == 0) ? 1 : 0;
  SDL_FreeSurface(surface);
  return ok;
}
