#include "main_ui.h"

lvgl_main_ui guider_main_ui;

static void make_camera_icon(lv_obj_t *parent) {
  lv_obj_t *body = lv_obj_create(parent);
  lv_obj_remove_style_all(body);
  lv_obj_set_size(body, 42, 29);
  lv_obj_center(body);
  lv_obj_set_style_bg_color(body, lv_color_hex(0xF7FAFC), 0);
  lv_obj_set_style_bg_opa(body, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(body, 7, 0);
  lv_obj_clear_flag(body, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_t *top = lv_obj_create(parent);
  lv_obj_remove_style_all(top);
  lv_obj_set_size(top, 19, 7);
  lv_obj_align(top, LV_ALIGN_CENTER, -7, -18);
  lv_obj_set_style_bg_color(top, lv_color_hex(0xF7FAFC), 0);
  lv_obj_set_style_bg_opa(top, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(top, 3, 0);
  lv_obj_t *lens = lv_obj_create(parent);
  lv_obj_remove_style_all(lens);
  lv_obj_set_size(lens, 17, 17);
  lv_obj_center(lens);
  lv_obj_set_style_radius(lens, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(lens, lv_color_hex(0x27364A), 0);
  lv_obj_set_style_bg_opa(lens, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(lens, 3, 0);
  lv_obj_set_style_border_color(lens, lv_color_hex(0x9FB3C8), 0);
}

static void make_gallery_icon(lv_obj_t *parent) {
  for (int i = 0; i < 2; ++i) {
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_remove_style_all(card);
    lv_obj_set_size(card, 39, 31);
    lv_obj_align(card, LV_ALIGN_CENTER, i ? 3 : -4, i ? 3 : -4);
    lv_obj_set_style_radius(card, 5, 0);
    lv_obj_set_style_bg_color(card, i ? lv_color_hex(0xF7FAFC) : lv_color_hex(0xA7C4BC), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(card, i ? 2 : 0, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0xDDEAE6), 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_CLICKABLE);
  }
  lv_obj_t *sun = lv_obj_create(parent);
  lv_obj_remove_style_all(sun);
  lv_obj_set_size(sun, 8, 8);
  lv_obj_align(sun, LV_ALIGN_CENTER, 9, -6);
  lv_obj_set_style_radius(sun, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(sun, lv_color_hex(0xE6A23C), 0);
  lv_obj_set_style_bg_opa(sun, LV_OPA_COVER, 0);
  lv_obj_t *mountain = lv_label_create(parent);
  lv_label_set_text(mountain, LV_SYMBOL_UP);
  lv_obj_set_style_text_color(mountain, lv_color_hex(0x4C6B63), 0);
  lv_obj_align(mountain, LV_ALIGN_CENTER, 1, 8);
}


static void menu_event(lv_event_t *event) {
  lv_event_code_t code = lv_event_get_code(event);
  lv_obj_t *target = lv_event_get_target(event);
  if (code == LV_EVENT_KEY) {
    uint32_t key = lv_event_get_key(event);
    lv_group_t *group = static_cast<lv_group_t *>(lv_obj_get_group(target));
    if (key == LV_KEY_LEFT) lv_group_focus_prev(group);
    if (key == LV_KEY_RIGHT) lv_group_focus_next(group);
  } else if (code == LV_EVENT_RELEASED) {
    lv_indev_t *input = lv_indev_get_act();
    if (input && lv_indev_get_key(input) != LV_KEY_ENTER) return;
    if (target == guider_main_ui.main_imgbtn_camera) {
      setup_scr_camera(&guider_camera_ui);
      lv_scr_load(guider_camera_ui.camera);
    } else {
      setup_scr_picture(&guider_picture_ui);
      lv_scr_load(guider_picture_ui.picture);
    }
    lv_obj_t *old = guider_main_ui.main;
    guider_main_ui.main = nullptr;
    lv_obj_del(old);
  }
}

static lv_obj_t *menu_card(lv_obj_t *parent, int x, int y, int w, int h,
                           const char *name, bool camera) {
  lv_obj_t *card = lv_btn_create(parent);
  lv_obj_remove_style_all(card);
  lv_obj_set_pos(card, x, y);
  lv_obj_set_size(card, w, h);
  lv_obj_set_style_radius(card, 8, 0);
  lv_obj_set_style_bg_color(card, lv_color_hex(UI_PANEL), 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(card, 2, LV_STATE_FOCUSED);
  lv_obj_set_style_border_color(card, lv_color_hex(UI_ACCENT), LV_STATE_FOCUSED);
  lv_obj_set_style_bg_color(card, lv_color_hex(0x353126), LV_STATE_PRESSED);
  lv_obj_set_style_outline_width(card, 0, LV_STATE_FOCUSED);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *art = ui_block(card, 0, 0, w, h - 23, UI_PANEL);
  lv_obj_set_style_bg_opa(art, LV_OPA_TRANSP, 0);
  if (camera) make_camera_icon(art); else make_gallery_icon(art);
  lv_obj_t *caption = ui_label(card, name);
  lv_obj_align(caption, LV_ALIGN_BOTTOM_MID, 0, -5);
  lv_obj_add_event_cb(card, menu_event, LV_EVENT_ALL, nullptr);
  return card;
}

void setup_scr_main(lvgl_main_ui *ui) {
  ui->main = lv_obj_create(nullptr);
  ui_screen_style(ui->main);
  const int w = lv_disp_get_hor_res(nullptr), h = lv_disp_get_ver_res(nullptr);
  ui->main_panel = ui->main;
  int card_w = (w - 30) / 2;
  ui->main_imgbtn_camera = menu_card(ui->main, 10, 29, card_w, h - 56,
                                    "SHOOT", true);
  ui->main_imgbtn_picture = menu_card(ui->main, 20 + card_w, 29, card_w, h - 56,
                                     "PLAYBACK", false);
  ui_status_bar(ui->main);
  ui_footer(ui->main, "aloeZ mini mini  /  OK Select");
  ui_attach_controls(ui->main, ui->main_imgbtn_camera, ui->main_imgbtn_picture);
  lv_scr_load(ui->main);
}
