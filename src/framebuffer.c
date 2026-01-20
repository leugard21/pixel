#include "framebuffer.h"

#include <stdlib.h>

static int iabs(int v) { return v < 0 ? -v : v; }
static int imin(int a, int b) { return a < b ? a : b; }
static int imax(int a, int b) { return a > b ? a : b; }

int fb_init(Framebuffer *fb, int w, int h) {
  fb->width = w;
  fb->height = h;
  fb->pixels = (uint32_t *)malloc(sizeof(uint32_t) * w * h);
  if (!fb->pixels)
    return 0;

  return 1;
}

void fb_destroy(Framebuffer *fb) {
  if (!fb)
    return;
  free(fb->pixels);
  fb->pixels = NULL;
  fb->width = 0;
  fb->height = 0;
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

void fb_draw_rect(Framebuffer *fb, int x0, int y0, int x1, int y1,
                  uint32_t color) {
  int left = imin(x0, x1);
  int right = imax(x0, x1);
  int top = imin(y0, y1);
  int bottom = imax(y0, y1);

  for (int x = left; x <= right; x++) {
    fb_put_pixel(fb, x, top, color);
    fb_put_pixel(fb, x, bottom, color);
  }
  for (int y = top; y <= bottom; y++) {
    fb_put_pixel(fb, left, y, color);
    fb_put_pixel(fb, right, y, color);
  }
}

void fb_fill_rect(Framebuffer *fb, int x0, int y0, int x1, int y1,
                  uint32_t color) {
  int left = imin(x0, x1);
  int right = imax(x0, x1);
  int top = imin(y0, y1);
  int bottom = imax(y0, y1);

  for (int y = top; y <= bottom; y++) {
    for (int x = left; x <= right; x++) {
      fb_put_pixel(fb, x, y, color);
    }
  }
}

static void circle_plot8(Framebuffer *fb, int cx, int cy, int x, int y,
                         uint32_t color) {
  fb_put_pixel(fb, cx + x, cy + y, color);
  fb_put_pixel(fb, cx - x, cy + y, color);
  fb_put_pixel(fb, cx + x, cy - y, color);
  fb_put_pixel(fb, cx - x, cy - y, color);
  fb_put_pixel(fb, cx + y, cy + x, color);
  fb_put_pixel(fb, cx - y, cy + x, color);
  fb_put_pixel(fb, cx + y, cy - x, color);
  fb_put_pixel(fb, cx - y, cy - x, color);
}

void fb_draw_circle(Framebuffer *fb, int cx, int cy, int radius,
                    uint32_t color) {
  if (radius <= 0) {
    fb_put_pixel(fb, cx, cy, color);
    return;
  }

  int x = radius;
  int y = 0;
  int err = 1 - x;

  while (x >= y) {
    circle_plot8(fb, cx, cy, x, y, color);
    y++;
    if (err < 0) {
      err += 2 * y + 1;
    } else {
      x--;
      err += 2 * (y - x) + 1;
    }
  }
}

void fb_fill_circle(Framebuffer *fb, int cx, int cy, int radius,
                    uint32_t color) {
  if (radius <= 0) {
    fb_put_pixel(fb, cx, cy, color);
    return;
  }

  int r2 = radius * radius;
  for (int y = -radius; y <= radius; y++) {
    int yy = y * y;
    for (int x = -radius; x <= radius; x++) {
      if (x * x + yy <= r2) {
        fb_put_pixel(fb, cx + x, cy + y, color);
      }
    }
  }
}
