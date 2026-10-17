#include "camera_ui.h"

void camera_build_view(lvgl_camera_ui *ui) {
  ui->camera = lv_obj_create(nullptr);
  ui_screen_style(ui->camera);
  uint32_t screen_width = lv_disp_get_hor_res(nullptr);
  uint32_t screen_height = lv_disp_get_ver_res(nullptr);
  ui->camera_video = lv_img_create(ui->camera);
  lv_obj_set_pos(ui->camera_video, 0, 0);
  lv_obj_set_size(ui->camera_video, screen_width, screen_height);

  // Four viewfinder corners leave the subject unobstructed.
  int x0 = 18, x1 = screen_width - 19, y0 = 31, y1 = screen_height - 29;
  for (int i = 0; i < 4; ++i) {
    int x = i & 1 ? x1 : x0, y = i & 2 ? y1 : y0;
    ui_block(ui->camera, x - (i & 1 ? 9 : 0), y, 10, 1, 0xF3F5F4);
    ui_block(ui->camera, x, y - (i & 2 ? 9 : 0), 1, 10, 0xF3F5F4);
  }
  ui_block(ui->camera, screen_width / 2 - 3, screen_height / 2, 7, 1, 0xF3F5F4);
  ui_block(ui->camera, screen_width / 2, screen_height / 2 - 3, 1, 7, 0xF3F5F4);
  ui_status_bar(ui->camera);
  ui->camera_feedback = ui_footer(ui->camera, "OK Shutter   " LV_SYMBOL_UP " Home");

  ui->camera_flash = ui_block(ui->camera, 0, 0, screen_width, screen_height, 0xF3F5F4);
  lv_obj_set_style_bg_opa(ui->camera_flash, LV_OPA_50, 0);
  lv_obj_add_flag(ui->camera_flash, LV_OBJ_FLAG_HIDDEN);
}
