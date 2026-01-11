#pragma once

#include <SDL2/SDL.h>

typedef struct {
  SDL_Texture *tex;
  int w;
  int h;
} UIText;

typedef struct {
  void *font;
} UI;

int ui_init(UI *ui, const char *font_path, int pt_size);
void ui_destroy(UI *ui);

int ui_text_make(UI *ui, SDL_Renderer *r, UIText *out, const char *text);
void ui_text_destroy(UIText *t);

void ui_draw_panel(SDL_Renderer *r, int x, int y, int w, int h);
void ui_draw_text(SDL_Renderer *r, const UIText *t, int x, int y);
