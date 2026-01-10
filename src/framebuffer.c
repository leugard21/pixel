#include "framebuffer.h"

#include <stdlib.h>

static int iabs(int v) { return v < 0 ? -v : v; }

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

uint32_t fb_get_pixel(const Framebuffer *fb, int x, int y, uint32_t fallback) {
  if (x < 0 || y < 0 || x >= fb->width || y >= fb->height)
    return fallback;
  return fb->pixels[y * fb->width + x];
}

void fb_draw_line(Framebuffer *fb, int x0, int y0, int x1, int y1,
                  uint32_t color) {
  int dx = iabs(x1 - x0);
  int sx = (x0 < x1) ? 1 : -1;
  int dy = -iabs(y1 - y0);
  int sy = (y0 < y1) ? 1 : -1;
  int err = dx + dy;

  for (;;) {
    fb_put_pixel(fb, x0, y0, color);
    if (x0 == x1 && y0 == y1)
      break;

    int e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y0 += sy;
    }
  }
}
