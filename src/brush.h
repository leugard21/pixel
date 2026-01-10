#pragma once

#include "framebuffer.h"
#include <stdint.h>

void brush_stamp_circle(Framebuffer *fb, int cx, int cy, int radius,
                        uint32_t color);
void brush_stroke_circle(Framebuffer *fb, int x0, int y0, int x1, int y1,
                         int radius, uint32_t color);
