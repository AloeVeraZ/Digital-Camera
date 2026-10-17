#include "public.h"
#include "ui_theme.h"
#include "storage_format.h"

static lv_obj_t *storage_label = nullptr;

lv_obj_t *ui_label(lv_obj_t *parent, const char *text, unsigned color) {
  lv_obj_t *label = lv_label_create(parent);
  lv_label_set_text(label, text);
  lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
  return label;
}

lv_obj_t *ui_block(lv_obj_t *parent, int x, int y, int w, int h, unsigned color) {
  lv_obj_t *obj = lv_obj_create(parent);
  lv_obj_remove_style_all(obj);
  lv_obj_set_pos(obj, x, y);
  lv_obj_set_size(obj, w, h);
  lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  return obj;
}

void ui_screen_style(lv_obj_t *screen) {
  lv_obj_remove_style_all(screen);
  lv_obj_set_size(screen, lv_disp_get_hor_res(nullptr), lv_disp_get_ver_res(nullptr));
  lv_obj_set_style_bg_color(screen, lv_color_hex(UI_BG), 0);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
}

static void delete_controls(lv_event_t *event) {
  lv_group_del(static_cast<lv_group_t *>(lv_event_get_user_data(event)));
}

void ui_attach_controls(lv_obj_t *screen, lv_obj_t *first, lv_obj_t *second) {
  lv_group_t *group = lv_group_create();
  lv_group_add_obj(group, first);
  if (second) lv_group_add_obj(group, second);
  lv_group_set_editing(group, true);
  lv_indev_set_group(indev_keypad, group);
  lv_obj_add_event_cb(screen, delete_controls, LV_EVENT_DELETE, group);
}

static void storage_deleted(lv_event_t *event) {
  if (storage_label == lv_event_get_target(event)) storage_label = nullptr;
}

void storage_status_refresh(void) {
  if (!storage_label) return;
  uint64_t total = SD_MMC.cardType() == CARD_NONE ? 0 : SD_MMC.totalBytes();
  if (!total) {
    lv_label_set_text(storage_label, LV_SYMBOL_SD_CARD " No SD");
    lv_obj_set_style_text_color(storage_label, lv_color_hex(0xF28B82), 0);
    return;
  }
  uint64_t used = SD_MMC.usedBytes();
  char usage[48];
  storage_format(usage, sizeof(usage), used, total);
  lv_label_set_text_fmt(storage_label, LV_SYMBOL_SD_CARD " %s", usage);
  lv_obj_set_style_text_color(storage_label,
      lv_color_hex(used >= total - total / 20 ? 0xF28B82 : UI_MUTED), 0);
}

void ui_status_bar(lv_obj_t *screen) {
  lv_obj_t *bar = ui_block(screen, 0, 0, lv_disp_get_hor_res(nullptr), 23, UI_BG);
  lv_obj_set_style_bg_opa(bar, LV_OPA_90, 0);
  storage_label = ui_label(bar, "");
  lv_obj_set_pos(storage_label, 5, 3);
  lv_obj_add_event_cb(storage_label, storage_deleted, LV_EVENT_DELETE, nullptr);
  storage_status_refresh();
  battery_status_attach(bar);
}

lv_obj_t *ui_footer(lv_obj_t *screen, const char *text) {
  lv_obj_t *bar = ui_block(screen, 0, lv_disp_get_ver_res(nullptr) - 20,
                         lv_disp_get_hor_res(nullptr), 20, UI_BG);
  lv_obj_set_style_bg_opa(bar, LV_OPA_90, 0);
  lv_obj_t *label = ui_label(bar, text, UI_MUTED);
  lv_obj_center(label);
  return label;
}
