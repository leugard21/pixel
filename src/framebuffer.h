#pragma once

#include <stdint.h>

#define ARGB(a, r, g, b)                                                       \
  (((uint32_t)(a) << 24) | ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) |      \
   ((uint32_t)(b)))

typedef struct {
  int width;
  int height;
  uint32_t *pixels;
} Framebuffer;

int fb_init(Framebuffer *fb, int w, int h);
void fb_destroy(Framebuffer *fb);

void fb_clear(Framebuffer *fb, uint32_t color);
void fb_put_pixel(Framebuffer *fb, int x, int y, uint32_t color);
uint32_t fb_get_pixel(const Framebuffer *fb, int x, int y, uint32_t fallback);

void fb_draw_line(Framebuffer *fb, int x0, int y0, int x1, int y1,
                  uint32_t color);
void fb_draw_rect(Framebuffer *fb, int x0, int y0, int x1, int y1,
                  uint32_t color);
void fb_fill_rect(Framebuffer *fb, int x0, int y0, int x1, int y1,
                  uint32_t color);

void fb_draw_circle(Framebuffer *fb, int cx, int cy, int radius,
                    uint32_t color);
void fb_fill_circle(Framebuffer *fb, int cx, int cy, int radius,
                    uint32_t color);
