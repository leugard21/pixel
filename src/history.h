#pragma once

#include "framebuffer.h"
#include <stdint.h>

typedef struct {
  uint32_t **items;
  int top;
  int capacity;
  int size;
  int width;
  int height;
} History;

int history_init(History *h, int capacity, int width, int height);
void history_destroy(History *h);

void history_clear(History *h);
int history_push(History *h, const Framebuffer *fb);
int history_pop(History *h, Framebuffer *fb);
