#include "brush.h"

static int iabs(int v) { return v < 0 ? -v : v; }

void brush_stamp_circle(Framebuffer *fb, int cx, int cy, int radius,
                        uint32_t color) {
  if (radius <= 0) {
    fb_put_pixel(fb, cx, cy, color);
    return;
  }

  int r2 = radius * radius;
  for (int y = -radius; y <= radius; y++) {
    for (int x = -radius; x <= radius; x++) {
      if (x * x + y * y <= r2) {
        fb_put_pixel(fb, cx + x, cy + y, color);
      }
    }
  }
}

void brush_stroke_circle(Framebuffer *fb, int x0, int y0, int x1, int y1,
                         int radius, uint32_t color) {
  int dx = iabs(x1 - x0);
  int sx = (x0 < x1) ? 1 : -1;
  int dy = -iabs(y1 - y0);
  int sy = (y0 < y1) ? 1 : -1;
  int err = dx + dy;

  for (;;) {
    brush_stamp_circle(fb, x0, y0, radius, color);
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
