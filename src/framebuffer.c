#include "framebuffer.h"

#include <stdlib.h>

int fb_init(Framebuffer *fb, int w, int h) {
  fb->width = w;
  fb->height = h;
  fb->pixels = (uint32_t *)malloc(sizeof(uint32_t) * w * h);
  if (!fb->pixels)
    return 0;

  return 1;
}

void fb_destroy(Framebuffer *fb) {
  free(fb->pixels);
  fb->pixels = 0;
}

void fb_clear(Framebuffer *fb, uint32_t color) {
  int count = fb->width * fb->height;
  for (int i = 0; i < count; i++) {
    fb->pixels[i] = color;
  }
}

void fb_put_pixel(Framebuffer *fb, int x, int y, uint32_t color) {
  if (x < 0 || y < 0 || x >= fb->width || y >= fb->height)
    return;

  fb->pixels[y * fb->width + x] = color;
}
