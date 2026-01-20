#pragma once

#include "ui.h"
#include <SDL2/SDL.h>
#include <stdint.h>

typedef struct {
  int x;
  int y;
  int w;
  int h;
} UIRect;

typedef enum {
  UI_EVENT_MOUSE_DOWN,
  UI_EVENT_MOUSE_UP,
  UI_EVENT_MOUSE_MOVE,
  UI_EVENT_MOUSE_ENTER,
  UI_EVENT_MOUSE_LEAVE
} UIEventType;

typedef struct {
  UIEventType type;
  int x;
  int y;
  int button;
} UIEvent;

typedef struct UIButton UIButton;
struct UIButton {
  UIRect bounds;
  char *label;
  UIText *text;
  int hovered;
  int pressed;
  int enabled;
  void (*on_click)(void *user_data);
  void *user_data;
};

typedef struct {
  UIRect bounds;
  uint32_t *colors;
  int color_count;
  int selected_index;
  int swatch_size;
  int hovered_index;
  void (*on_color_change)(uint32_t color, void *user_data);
  void *user_data;
} UIColorPicker;

typedef struct {
  UIRect bounds;
  UIButton *buttons;
  int button_count;
  int selected_index;
  int horizontal;
  int spacing;
} UIToolbar;

int ui_rect_contains(const UIRect *rect, int x, int y);

int ui_button_init(UIButton *btn, int x, int y, int w, int h,
                   const char *label);
void ui_button_destroy(UIButton *btn);
void ui_button_set_callback(UIButton *btn, void (*on_click)(void *),
                            void *user_data);
int ui_button_handle_event(UIButton *btn, const UIEvent *event);
void ui_button_render(const UIButton *btn, SDL_Renderer *r, UI *ui);

int ui_color_picker_init(UIColorPicker *picker, int x, int y, int swatch_size,
                         const uint32_t *colors, int color_count);
void ui_color_picker_destroy(UIColorPicker *picker);
void ui_color_picker_set_callback(UIColorPicker *picker,
                                  void (*on_color_change)(uint32_t, void *),
                                  void *user_data);
void ui_color_picker_set_selected(UIColorPicker *picker, int index);
int ui_color_picker_handle_event(UIColorPicker *picker, const UIEvent *event);
void ui_color_picker_render(const UIColorPicker *picker, SDL_Renderer *r);

int ui_toolbar_init(UIToolbar *toolbar, int x, int y, int horizontal);
void ui_toolbar_destroy(UIToolbar *toolbar);
int ui_toolbar_add_button(UIToolbar *toolbar, const char *label,
                          void (*on_click)(void *), void *user_data);
void ui_toolbar_set_selected(UIToolbar *toolbar, int index);
int ui_toolbar_handle_event(UIToolbar *toolbar, const UIEvent *event, UI *ui);
void ui_toolbar_render(const UIToolbar *toolbar, SDL_Renderer *r, UI *ui);

typedef struct {
  UIRect bounds;
  int min_value;
  int max_value;
  int current_value;
  int dragging;
  char *label;
  UIText *label_text;
  void (*on_value_change)(int value, void *user_data);
  void *user_data;
} UISlider;

int ui_slider_init(UISlider *slider, int x, int y, int w, int h,
                   const char *label, int min_val, int max_val,
                   int initial_val);
void ui_slider_destroy(UISlider *slider);
void ui_slider_set_callback(UISlider *slider,
                            void (*on_value_change)(int, void *),
                            void *user_data);
void ui_slider_set_value(UISlider *slider, int value);
int ui_slider_get_value(const UISlider *slider);
int ui_slider_handle_event(UISlider *slider, const UIEvent *event);
void ui_slider_render(const UISlider *slider, SDL_Renderer *r, UI *ui);

typedef struct {
  UIRect bounds;
  char text[256];
  UIText *rendered_text;
  int dirty;
} UIStatusBar;

int ui_status_bar_init(UIStatusBar *bar, int x, int y, int w, int h);
void ui_status_bar_destroy(UIStatusBar *bar);
void ui_status_bar_set_text(UIStatusBar *bar, const char *text);
void ui_status_bar_render(const UIStatusBar *bar, SDL_Renderer *r, UI *ui);
