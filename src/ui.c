#include "ui.h"

#include <SDL2/SDL_ttf.h>
#include <stdio.h>

int ui_init(UI *ui, const char *font_path, int pt_size) {
  if (TTF_Init() != 0)
    return 0;

  TTF_Font *font = TTF_OpenFont(font_path, pt_size);
  if (!font) {
    TTF_Quit();
    return 0;
  }

  ui->font = font;
  return 1;
}

void ui_destroy(UI *ui) {
  if (!ui)
    return;

  if (ui->font) {
    TTF_CloseFont((TTF_Font *)ui->font);
    ui->font = NULL;
  }
  TTF_Quit();
}

int ui_text_make(UI *ui, SDL_Renderer *r, UIText *out, const char *text) {
  if (!ui || !ui->font || !out || !text)
    return 0;

  SDL_Color c = {255, 255, 255, 220};
  SDL_Surface *s = TTF_RenderUTF8_Blended((TTF_Font *)ui->font, text, c);
  if (!s)
    return 0;

  SDL_Texture *t = SDL_CreateTextureFromSurface(r, s);
  if (!t) {
    SDL_FreeSurface(s);
    return 0;
  }

  out->tex = t;
  out->w = s->w;
  out->h = s->h;
  SDL_FreeSurface(s);
  return 1;
}

void ui_text_destroy(UIText *t) {
  if (!t)
    return;
  if (t->tex)
    SDL_DestroyTexture(t->tex);
  t->tex = NULL;
  t->w = 0;
  t->h = 0;
}

void ui_draw_panel(SDL_Renderer *r, int x, int y, int w, int h) {
  SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

  SDL_Rect panel = {x, y, w, h};
  SDL_SetRenderDrawColor(r, 0, 0, 0, 150);
  SDL_RenderFillRect(r, &panel);

  SDL_SetRenderDrawColor(r, 255, 255, 255, 180);
  SDL_RenderDrawRect(r, &panel);
}

void ui_draw_text(SDL_Renderer *r, const UIText *t, int x, int y) {
  SDL_Rect dst = {x, y, t->w, t->h};
  SDL_RenderCopy(r, t->tex, NULL, &dst);
}
