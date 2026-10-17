#pragma once
#include "public.h"

struct lvgl_picture_ui {
  lv_obj_t *picture = nullptr;
  lv_obj_t *picture_show = nullptr;
  lv_obj_t *picture_counter = nullptr;
  lv_obj_t *grid = nullptr;
  lv_obj_t *message = nullptr;
  lv_timer_t *picture_timer = nullptr;
};
extern lvgl_picture_ui guider_picture_ui;
void setup_scr_picture(lvgl_picture_ui *ui);
void picture_display(const char *name);
