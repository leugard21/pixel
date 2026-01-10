#pragma once

#include <stdint.h>

typedef struct {
  int width;
  int height;
  uint32_t *pixels;
} Framebuffer;

int fb_init(Framebuffer *fb, int w, int h);
void fb_destroy(Framebuffer *fb);

void fb_clear(Framebuffer *fb, uint32_t color);
void fb_put_pixel(Framebuffer *fb, int x, int y, uint32_t color);
