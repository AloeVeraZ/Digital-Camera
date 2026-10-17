#pragma once
#include "lvgl.h"
#include <stddef.h>

struct PhotoImage {
  uint8_t *pixels = nullptr;
  size_t capacity = 0;
  lv_img_dsc_t image = {};
};
// Fit the whole image, or center-crop it into a square thumbnail.
bool photo_image_load(const char *path, PhotoImage &photo, int max_w, int max_h, bool crop);
void photo_image_free(PhotoImage &photo);
