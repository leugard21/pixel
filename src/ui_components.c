#include "ui_components.h"

#include <stdlib.h>
#include <string.h>

int ui_rect_contains(const UIRect *rect, int x, int y) {
  return x >= rect->x && x < rect->x + rect->w && y >= rect->y &&
         y < rect->y + rect->h;
}

int ui_button_init(UIButton *btn, int x, int y, int w, int h,
                   const char *label) {
  if (!btn)
    return 0;

  btn->bounds.x = x;
  btn->bounds.y = y;
  btn->bounds.w = w;
  btn->bounds.h = h;

  btn->label = NULL;
  if (label) {
    btn->label = strdup(label);
    if (!btn->label)
      return 0;
  }

  btn->text = NULL;
  btn->hovered = 0;
  btn->pressed = 0;
  btn->enabled = 1;
  btn->on_click = NULL;
  btn->user_data = NULL;

  return 1;
}

void ui_button_destroy(UIButton *btn) {
  if (!btn)
    return;

  free(btn->label);
  btn->label = NULL;

  if (btn->text) {
    ui_text_destroy(btn->text);
    free(btn->text);
    btn->text = NULL;
  }
}

void ui_button_set_callback(UIButton *btn, void (*on_click)(void *),
                            void *user_data) {
  if (!btn)
    return;
  btn->on_click = on_click;
  btn->user_data = user_data;
}

int ui_button_handle_event(UIButton *btn, const UIEvent *event) {
  if (!btn || !btn->enabled || !event)
    return 0;

  int inside = ui_rect_contains(&btn->bounds, event->x, event->y);

  switch (event->type) {
  case UI_EVENT_MOUSE_MOVE:
    btn->hovered = inside;
    return inside;

  case UI_EVENT_MOUSE_DOWN:
    if (inside) {
      btn->pressed = 1;
      return 1;
    }
    break;

  case UI_EVENT_MOUSE_UP:
    if (btn->pressed && inside) {
      btn->pressed = 0;
      if (btn->on_click)
        btn->on_click(btn->user_data);
      return 1;
    }
    btn->pressed = 0;
    break;

  default:
    break;
  }

  return 0;
}

void ui_button_render(const UIButton *btn, SDL_Renderer *r, UI *ui) {
  if (!btn || !r)
    return;

  SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

  SDL_Rect rect = {btn->bounds.x, btn->bounds.y, btn->bounds.w, btn->bounds.h};

  if (!btn->enabled) {
    SDL_SetRenderDrawColor(r, 60, 60, 60, 150);
    SDL_RenderFillRect(r, &rect);
    SDL_SetRenderDrawColor(r, 100, 100, 100, 180);
    SDL_RenderDrawRect(r, &rect);
  } else if (btn->pressed) {
    SDL_SetRenderDrawColor(r, 80, 120, 180, 200);
    SDL_RenderFillRect(r, &rect);
    SDL_SetRenderDrawColor(r, 120, 160, 220, 255);
    SDL_RenderDrawRect(r, &rect);
  } else if (btn->hovered) {
    SDL_SetRenderDrawColor(r, 60, 90, 140, 180);
    SDL_RenderFillRect(r, &rect);
    SDL_SetRenderDrawColor(r, 100, 140, 200, 220);
    SDL_RenderDrawRect(r, &rect);
  } else {
    SDL_SetRenderDrawColor(r, 40, 40, 40, 150);
    SDL_RenderFillRect(r, &rect);
    SDL_SetRenderDrawColor(r, 180, 180, 180, 180);
    SDL_RenderDrawRect(r, &rect);
  }

  if (btn->label && ui && ui->font) {
    if (!btn->text) {
      UIButton *mutable_btn = (UIButton *) btn;
      mutable_btn->text = (UIText *) malloc(sizeof(UIText));
      if (mutable_btn->text) {
        if (!ui_text_make(ui, r, mutable_btn->text, btn->label)) {
          free(mutable_btn->text);
          mutable_btn->text = NULL;
        }
      }
    }

    if (btn->text && btn->text->tex) {
      int text_x = btn->bounds.x + (btn->bounds.w - btn->text->w) / 2;
      int text_y = btn->bounds.y + (btn->bounds.h - btn->text->h) / 2;
      ui_draw_text(r, btn->text, text_x, text_y);
    }
  }
}

int ui_color_picker_init(UIColorPicker *picker, int x, int y, int swatch_size,
                         const uint32_t *colors, int color_count) {
  if (!picker || !colors || color_count <= 0)
    return 0;

  picker->colors = (uint32_t *) malloc(sizeof(uint32_t) * color_count);
  if (!picker->colors)
    return 0;

  memcpy(picker->colors, colors, sizeof(uint32_t) * color_count);
  picker->color_count = color_count;
  picker->swatch_size = swatch_size;
  picker->selected_index = 0;
  picker->hovered_index = -1;
  picker->on_color_change = NULL;
  picker->user_data = NULL;

  picker->bounds.x = x;
  picker->bounds.y = y;
  picker->bounds.w = swatch_size * color_count + (color_count - 1) * 4;
  picker->bounds.h = swatch_size;

  return 1;
}

void ui_color_picker_destroy(UIColorPicker *picker) {
  if (!picker)
    return;
  free(picker->colors);
  picker->colors = NULL;
}

void ui_color_picker_set_callback(UIColorPicker *picker,
                                  void (*on_color_change)(uint32_t, void *),
                                  void *user_data) {
  if (!picker)
    return;
  picker->on_color_change = on_color_change;
  picker->user_data = user_data;
}

void ui_color_picker_set_selected(UIColorPicker *picker, int index) {
  if (!picker || index < 0 || index >= picker->color_count)
    return;
  picker->selected_index = index;
}

int ui_color_picker_handle_event(UIColorPicker *picker, const UIEvent *event) {
  if (!picker || !event)
    return 0;

  int inside = ui_rect_contains(&picker->bounds, event->x, event->y);

  if (event->type == UI_EVENT_MOUSE_MOVE) {
    if (inside) {
      int offset_x = event->x - picker->bounds.x;
      int index = offset_x / (picker->swatch_size + 4);
      if (index >= 0 && index < picker->color_count) {
        picker->hovered_index = index;
        return 1;
      }
    }
    picker->hovered_index = -1;
    return inside;
  }

  if (event->type == UI_EVENT_MOUSE_DOWN && inside) {
    int offset_x = event->x - picker->bounds.x;
    int index = offset_x / (picker->swatch_size + 4);
    if (index >= 0 && index < picker->color_count) {
      picker->selected_index = index;
      if (picker->on_color_change)
        picker->on_color_change(picker->colors[index], picker->user_data);
      return 1;
    }
  }

  return 0;
}

void ui_color_picker_render(const UIColorPicker *picker, SDL_Renderer *r) {
  if (!picker || !r)
    return;

  SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

  for (int i = 0; i < picker->color_count; i++) {
    int sx = picker->bounds.x + i * (picker->swatch_size + 4);
    int sy = picker->bounds.y;

    SDL_Rect swatch = {sx, sy, picker->swatch_size, picker->swatch_size};

    uint32_t c = picker->colors[i];
    Uint8 a = (Uint8) ((c >> 24) & 0xFF);
    Uint8 r_val = (Uint8) ((c >> 16) & 0xFF);
    Uint8 g = (Uint8) ((c >> 8) & 0xFF);
    Uint8 b = (Uint8) (c & 0xFF);

    SDL_SetRenderDrawColor(r, r_val, g, b, a);
    SDL_RenderFillRect(r, &swatch);

    if (i == picker->selected_index) {
      SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
      SDL_RenderDrawRect(r, &swatch);
      SDL_Rect inner = {sx + 1, sy + 1, picker->swatch_size - 2,
                        picker->swatch_size - 2};
      SDL_RenderDrawRect(r, &inner);
    } else if (i == picker->hovered_index) {
      SDL_SetRenderDrawColor(r, 200, 200, 200, 200);
      SDL_RenderDrawRect(r, &swatch);
    } else {
      SDL_SetRenderDrawColor(r, 150, 150, 150, 150);
      SDL_RenderDrawRect(r, &swatch);
    }
  }
}

int ui_toolbar_init(UIToolbar *toolbar, int x, int y, int horizontal) {
  if (!toolbar)
    return 0;

  toolbar->bounds.x = x;
  toolbar->bounds.y = y;
  toolbar->bounds.w = 0;
  toolbar->bounds.h = 0;
  toolbar->buttons = NULL;
  toolbar->button_count = 0;
  toolbar->selected_index = -1;
  toolbar->horizontal = horizontal;
  toolbar->spacing = 4;

  return 1;
}

void ui_toolbar_destroy(UIToolbar *toolbar) {
  if (!toolbar)
    return;

  for (int i = 0; i < toolbar->button_count; i++) {
    ui_button_destroy(&toolbar->buttons[i]);
  }
  free(toolbar->buttons);
  toolbar->buttons = NULL;
  toolbar->button_count = 0;
}

int ui_toolbar_add_button(UIToolbar *toolbar, const char *label,
                          void (*on_click)(void *), void *user_data) {
  if (!toolbar)
    return 0;

  UIButton *new_buttons = (UIButton *) realloc(
      toolbar->buttons, sizeof(UIButton) * (toolbar->button_count + 1));
  if (!new_buttons)
    return 0;

  toolbar->buttons = new_buttons;

  int btn_w = 80;
  int btn_h = 30;
  int x, y;

  if (toolbar->horizontal) {
    x = toolbar->bounds.x + toolbar->button_count * (btn_w + toolbar->spacing);
    y = toolbar->bounds.y;
    toolbar->bounds.w = (toolbar->button_count + 1) * btn_w +
                        toolbar->button_count * toolbar->spacing;
    toolbar->bounds.h = btn_h;
  } else {
    x = toolbar->bounds.x;
    y = toolbar->bounds.y + toolbar->button_count * (btn_h + toolbar->spacing);
    toolbar->bounds.w = btn_w;
    toolbar->bounds.h = (toolbar->button_count + 1) * btn_h +
                        toolbar->button_count * toolbar->spacing;
  }

  UIButton *btn = &toolbar->buttons[toolbar->button_count];
  if (!ui_button_init(btn, x, y, btn_w, btn_h, label)) {
    return 0;
  }

  ui_button_set_callback(btn, on_click, user_data);
  toolbar->button_count++;

  return 1;
}

void ui_toolbar_set_selected(UIToolbar *toolbar, int index) {
  if (!toolbar || index < -1 || index >= toolbar->button_count)
    return;
  toolbar->selected_index = index;
}

int ui_toolbar_handle_event(UIToolbar *toolbar, const UIEvent *event, UI *ui) {
  if (!toolbar || !event)
    return 0;

  (void) ui;

  for (int i = 0; i < toolbar->button_count; i++) {
    if (ui_button_handle_event(&toolbar->buttons[i], event)) {
      if (event->type == UI_EVENT_MOUSE_UP) {
        toolbar->selected_index = i;
      }
      return 1;
    }
  }

  return 0;
}

void ui_toolbar_render(const UIToolbar *toolbar, SDL_Renderer *r, UI *ui) {
  if (!toolbar || !r)
    return;

  for (int i = 0; i < toolbar->button_count; i++) {
    ui_button_render(&toolbar->buttons[i], r, ui);

    if (i == toolbar->selected_index) {
      SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
      SDL_Rect highlight = {
          toolbar->buttons[i].bounds.x - 2, toolbar->buttons[i].bounds.y - 2,
          toolbar->buttons[i].bounds.w + 4, toolbar->buttons[i].bounds.h + 4};
      SDL_SetRenderDrawColor(r, 100, 200, 255, 200);
      SDL_RenderDrawRect(r, &highlight);
    }
  }
}
