#pragma once
#include "lvgl.h"

constexpr unsigned UI_BG = 0x0B0F12;
constexpr unsigned UI_PANEL = 0x192126;
constexpr unsigned UI_MUTED = 0x98A6AC;
constexpr unsigned UI_ACCENT = 0xE6B85C;
lv_obj_t *ui_label(lv_obj_t *parent, const char *text, unsigned color = 0xF3F5F4);
lv_obj_t *ui_block(lv_obj_t *parent, int x, int y, int w, int h, unsigned color);
void ui_screen_style(lv_obj_t *screen);
void ui_attach_controls(lv_obj_t *screen, lv_obj_t *first, lv_obj_t *second = nullptr);
void ui_status_bar(lv_obj_t *screen);
lv_obj_t *ui_footer(lv_obj_t *screen, const char *text);
void storage_status_refresh(void);
