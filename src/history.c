#include "history.h"

#include <stdlib.h>
#include <string.h>

int history_init(History *h, int capacity, int width, int height) {
  h->items = (uint32_t **)malloc(sizeof(uint32_t *) * capacity);
  if (!h->items)
    return 0;

  h->capacity = capacity;
  h->top = -1;
  h->size = 0;
  h->width = width;
  h->height = height;

  return 1;
}

void history_destroy(History *h) {
  history_clear(h);
  free(h->items);
  h->items = 0;
}

void history_clear(History *h) {
  for (int i = 0; i < h->size; i++) {
    free(h->items[i]);
  }

  h->top = -1;
  h->size = 0;
}

int history_push(History *h, const Framebuffer *fb) {
  if (h->size == h->capacity) {
    free(h->items[0]);
    for (int i = 1; i < h->size; i++) {
      h->items[i - 1] = h->items[i];
    }
    h->size--;
    h->top--;
  }

  uint32_t *copy = (uint32_t *)malloc(sizeof(uint32_t) * h->width * h->height);
  if (!copy)
    return 0;

  memcpy(copy, fb->pixels, sizeof(uint32_t) * h->width * h->height);
  h->items[++h->top] = copy;
  h->size++;
  return 1;
}

int history_pop(History *h, Framebuffer *fb) {
  if (h->top < 0)
    return 0;

  uint32_t *state = h->items[h->top--];
  memcpy(fb->pixels, state, sizeof(uint32_t) * h->width * h->height);
  free(state);
  h->size--;

  return 1;
}
