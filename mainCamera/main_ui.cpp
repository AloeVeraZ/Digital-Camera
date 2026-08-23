#include "main_ui.h"

lvgl_main_ui guider_main_ui;  // main ui structure

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

static void main_imgbtn_camera_event_cb(lv_event_t *event) {
  lv_event_code_t code = lv_event_get_code(event);
  
  if (code == LV_EVENT_KEY) {
    uint32_t key = lv_event_get_key(event);
    if (key == LV_KEY_LEFT) {
      lv_group_t *g = (lv_group_t *)lv_obj_get_group(lv_event_get_target(event));
      if (g) lv_group_focus_prev(g);
    } 
    else if (key == LV_KEY_RIGHT) {
      lv_group_t *g = (lv_group_t *)lv_obj_get_group(lv_event_get_target(event));
      if (g) lv_group_focus_next(g);
    }
  }
  else if (code == LV_EVENT_RELEASED) {
    lv_indev_t *indev = lv_indev_get_act();
    if (indev && lv_indev_get_key(indev) != LV_KEY_ENTER) {
      return;
    }
    
    Serial.println("Jumping to camera screen!");
    if (!lv_obj_is_valid(guider_camera_ui.camera))
      setup_scr_camera(&guider_camera_ui);
    lv_scr_load(guider_camera_ui.camera);
    
    uint32_t w = lv_disp_get_hor_res(NULL);
    uint32_t h = lv_disp_get_ver_res(NULL);
    create_camera_task(w, h);
    lv_obj_del(guider_main_ui.main);
  }
  else if (code == LV_EVENT_FOCUSED) {
    lv_obj_scroll_to_view_recursive(lv_event_get_target(event), LV_ANIM_ON);
  }
}

static void main_imgbtn_picture_event_cb(lv_event_t *event) {
  lv_event_code_t code = lv_event_get_code(event);
  
  if (code == LV_EVENT_KEY) {
    uint32_t key = lv_event_get_key(event);
    if (key == LV_KEY_LEFT) {
      lv_group_t *g = (lv_group_t *)lv_obj_get_group(lv_event_get_target(event));
      if (g) lv_group_focus_prev(g);
    } 
    else if (key == LV_KEY_RIGHT) {
      lv_group_t *g = (lv_group_t *)lv_obj_get_group(lv_event_get_target(event));
      if (g) lv_group_focus_next(g);
    }
  }
  else if (code == LV_EVENT_RELEASED) {
    lv_indev_t *indev = lv_indev_get_act();
    if (indev && lv_indev_get_key(indev) != LV_KEY_ENTER) {
      return;
    }
    
    Serial.println("Jumping to picture gallery!");
    if (!lv_obj_is_valid(guider_picture_ui.picture))
      setup_scr_picture(&guider_picture_ui);
    lv_scr_load(guider_picture_ui.picture);
    lv_obj_del(guider_main_ui.main);
  }
  else if (code == LV_EVENT_FOCUSED) {
    lv_obj_scroll_to_view_recursive(lv_event_get_target(event), LV_ANIM_ON);
  }
}

void setup_scr_main(lvgl_main_ui *ui) {
  ui->main = lv_obj_create(NULL);
  lv_obj_clear_flag(ui->main, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(ui->main, lv_color_hex(0xE4E7EB), 0);
  
  ui->main_panel = lv_obj_create(ui->main);
  lv_obj_set_size(ui->main_panel, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
  lv_obj_set_flex_flow(ui->main_panel, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(ui->main_panel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(ui->main_panel, 24, 0);
  lv_obj_set_style_bg_color(ui->main_panel, lv_color_hex(0xE4E7EB), 0);
  lv_obj_set_style_bg_opa(ui->main_panel, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(ui->main_panel, 0, 0);
  lv_obj_set_scroll_snap_x(ui->main_panel, LV_SCROLL_SNAP_CENTER);
  lv_obj_set_scrollbar_mode(ui->main_panel, LV_SCROLLBAR_MODE_OFF);
  lv_obj_align(ui->main_panel, LV_ALIGN_CENTER, 0, 0);

  static lv_style_t pressed_style;
  lv_style_init(&pressed_style);
  lv_style_set_transform_zoom(&pressed_style, 240);

  static lv_style_t focused_style;
  lv_style_init(&focused_style);
  lv_style_set_border_width(&focused_style, 3);
  lv_style_set_border_color(&focused_style, lv_color_hex(0x2563EB));
  lv_style_set_shadow_width(&focused_style, 12);
  lv_style_set_shadow_opa(&focused_style, LV_OPA_20);

  // 1. Setup Camera Button
  ui->main_imgbtn_camera = lv_btn_create(ui->main_panel);
  lv_obj_set_size(ui->main_imgbtn_camera, 80, 80);
  lv_obj_set_style_radius(ui->main_imgbtn_camera, 18, 0);
  lv_obj_set_style_bg_color(ui->main_imgbtn_camera, lv_color_hex(0x27364A), 0);
  make_camera_icon(ui->main_imgbtn_camera);
  lv_obj_add_style(ui->main_imgbtn_camera, &pressed_style, LV_STATE_PRESSED | LV_PART_MAIN);
  lv_obj_add_style(ui->main_imgbtn_camera, &focused_style, LV_STATE_FOCUSED | LV_PART_MAIN);
  lv_obj_add_event_cb(ui->main_imgbtn_camera, main_imgbtn_camera_event_cb, LV_EVENT_ALL, NULL);

  // 2. Setup Storage/Picture Button
  ui->main_imgbtn_picture = lv_btn_create(ui->main_panel);
  lv_obj_set_size(ui->main_imgbtn_picture, 80, 80);
  lv_obj_set_style_radius(ui->main_imgbtn_picture, 18, 0);
  lv_obj_set_style_bg_color(ui->main_imgbtn_picture, lv_color_hex(0x4C6B63), 0);
  make_gallery_icon(ui->main_imgbtn_picture);
  lv_obj_add_style(ui->main_imgbtn_picture, &pressed_style, LV_STATE_PRESSED | LV_PART_MAIN);
  lv_obj_add_style(ui->main_imgbtn_picture, &focused_style, LV_STATE_FOCUSED | LV_PART_MAIN);
  lv_obj_add_event_cb(ui->main_imgbtn_picture, main_imgbtn_picture_event_cb, LV_EVENT_ALL, NULL);

  lv_obj_update_snap(ui->main_panel, LV_ANIM_OFF);
  lv_obj_scroll_to_view_recursive(ui->main_imgbtn_camera, LV_ANIM_OFF);

  lv_group_t *group = lv_group_create();
  lv_group_add_obj(group, ui->main_imgbtn_camera);
  lv_group_add_obj(group, ui->main_imgbtn_picture);
  lv_group_set_editing(group, false);
  lv_indev_set_group(indev_keypad, group);

  battery_status_attach(ui->main);

  lv_scr_load(ui->main);
}
