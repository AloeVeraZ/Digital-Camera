#include "picture_ui.h"
#include "photo_image.h"
#include "gallery_navigation.h"
#include <algorithm>
#include <vector>

lvgl_picture_ui guider_picture_ui;
static GalleryNavigation nav;
static std::vector<String> picture_files;
static PhotoImage full_images[2], thumbnails[GalleryNavigation::page_size];
static String full_names[2];
static int next_slot = 0, loaded_index = -1, loaded_page = -1, next_thumb = 0;
static int tile_size = 40;
static lv_obj_t *tiles[GalleryNavigation::page_size], *tile_images[GalleryNavigation::page_size];
static lv_obj_t *tile_errors[GalleryNavigation::page_size];

static int photo_number(const String &name) {
  int number = -1;
  return sscanf(name.c_str(), "photo%d", &number) == 1 ? number : -1;
}

static void cache_picture_list() {
  picture_files.clear();
  for (const auto &entry : list_dir(PICTURE_FOLDER, 0)) {
    String lower = entry.name;
    lower.toLowerCase();
    if (!entry.isDirectory && (lower.endsWith(".bmp") || lower.endsWith(".jpg") ||
                               lower.endsWith(".jpeg"))) picture_files.push_back(entry.name);
  }
  std::sort(picture_files.begin(), picture_files.end(), [](const String &a, const String &b) {
    int an = photo_number(a), bn = photo_number(b);
    if (an >= 0 && bn >= 0 && an != bn) return an > bn;
    if ((an >= 0) != (bn >= 0)) return an >= 0;
    return a.compareTo(b) > 0;
  });
  nav = GalleryNavigation{};
  nav.count = picture_files.size();
}

static void update_chrome() {
  auto &ui = guider_picture_ui;
  if (nav.grid) {
    lv_obj_add_flag(ui.picture_show, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui.grid, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui.message, LV_OBJ_FLAG_HIDDEN);
    for (int i = 0; i < GalleryNavigation::page_size; ++i) {
      lv_obj_set_style_border_color(tiles[i], lv_color_hex(
          nav.page_start() + i == nav.index ? UI_ACCENT : UI_PANEL), 0);
    }
    lv_label_set_text_fmt(ui.picture_counter, "%d/%d   " LV_SYMBOL_UP " Open   OK View",
                         nav.index + 1, nav.count);
  } else {
    lv_obj_add_flag(ui.grid, LV_OBJ_FLAG_HIDDEN);
    if (nav.count) {
      lv_obj_clear_flag(ui.picture_show, LV_OBJ_FLAG_HIDDEN);
      lv_label_set_text_fmt(ui.picture_counter, "%d/%d   " LV_SYMBOL_DOWN " Grid   " LV_SYMBOL_UP " Home",
                           nav.index + 1, nav.count);
    } else lv_label_set_text(ui.picture_counter, LV_SYMBOL_UP " Home to start shooting");
  }
  lv_obj_center(ui.picture_counter);
}

void picture_display(const char *name) {
  auto &ui = guider_picture_ui;
  lv_obj_add_flag(ui.message, LV_OBJ_FLAG_HIDDEN);
  int slot = -1;
  for (int i = 0; i < 2; ++i)
    if (full_names[i] == name && full_images[i].pixels) slot = i;
  if (slot < 0) {
    slot = next_slot;
    next_slot = 1 - next_slot;
    // Never overwrite the buffer currently being displayed.
    lv_img_set_src(ui.picture_show, nullptr);
    full_names[slot] = "";
    String path = String(PICTURE_FOLDER) + "/" + name;
    if (!photo_image_load(path.c_str(), full_images[slot], lv_disp_get_hor_res(nullptr),
                           lv_disp_get_ver_res(nullptr), false)) {
      lv_label_set_text(ui.message, "Cannot open photo");
      lv_obj_clear_flag(ui.message, LV_OBJ_FLAG_HIDDEN);
      return;
    }
    full_names[slot] = name;
  }
  lv_img_set_src(ui.picture_show, &full_images[slot].image);
  lv_obj_center(ui.picture_show);
}

static void prepare_grid_page() {
  loaded_page = nav.page_start();
  next_thumb = 0;
  for (int i = 0; i < GalleryNavigation::page_size; ++i) {
    lv_img_set_src(tile_images[i], nullptr);
    lv_obj_add_flag(tile_errors[i], LV_OBJ_FLAG_HIDDEN);
    if (loaded_page + i < nav.count) lv_obj_clear_flag(tiles[i], LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(tiles[i], LV_OBJ_FLAG_HIDDEN);
  }
}

// Load one thumbnail per timer tick so button handling keeps running between files.
static void picture_tick(lv_timer_t *) {
  if (!nav.grid) {
    if (nav.count && loaded_index != nav.index) {
      picture_display(picture_files[nav.index].c_str());
      loaded_index = nav.index;
    }
    return;
  }
  if (loaded_page != nav.page_start()) prepare_grid_page();
  if (next_thumb >= GalleryNavigation::page_size || loaded_page + next_thumb >= nav.count) return;
  int slot = next_thumb++;
  String path = String(PICTURE_FOLDER) + "/" + picture_files[loaded_page + slot];
  if (photo_image_load(path.c_str(), thumbnails[slot], tile_size - 4, tile_size - 4, true)) {
    lv_img_set_src(tile_images[slot], &thumbnails[slot].image);
    lv_obj_center(tile_images[slot]);
  } else lv_obj_clear_flag(tile_errors[slot], LV_OBJ_FLAG_HIDDEN);
}

static void gallery_deleted(lv_event_t *) {
  auto &ui = guider_picture_ui;
  if (ui.picture_timer) lv_timer_del(ui.picture_timer);
  ui.picture_timer = nullptr;
  // Parent DELETE fires before child deletion: detach every image before freeing.
  lv_img_set_src(ui.picture_show, nullptr);
  for (int i = 0; i < GalleryNavigation::page_size; ++i) {
    lv_img_set_src(tile_images[i], nullptr);
    photo_image_free(thumbnails[i]);
  }
  for (int i = 0; i < 2; ++i) { photo_image_free(full_images[i]); full_names[i] = ""; }
  picture_files.clear();
  ui.picture = nullptr;
}

static void picture_event(lv_event_t *event) {
  uint32_t key = lv_event_get_key(event);
  bool was_grid = nav.grid;
  if (key == 'B' && nav.up_exits()) {
    lv_obj_t *old = guider_picture_ui.picture;
    setup_scr_main(&guider_main_ui);
    lv_obj_del(old);
    return;
  }
  if (key == 'D') nav.down();
  if (key == LV_KEY_ENTER && nav.grid) nav.grid = false;
  if (key == LV_KEY_LEFT) nav.move(-1);
  if (key == LV_KEY_RIGHT) nav.move(1);
  if (was_grid && !nav.grid) loaded_index = -1;
  if (!was_grid && nav.grid && loaded_page != nav.page_start()) prepare_grid_page();
  update_chrome();
}

void setup_scr_picture(lvgl_picture_ui *ui) {
  ui->picture = lv_obj_create(nullptr);
  ui_screen_style(ui->picture);
  int w = lv_disp_get_hor_res(nullptr), h = lv_disp_get_ver_res(nullptr);
  ui->picture_show = lv_img_create(ui->picture);
  ui->message = ui_label(ui->picture, "Loading photos...", UI_MUTED);
  lv_obj_center(ui->message);
  ui->grid = ui_block(ui->picture, 0, 24, w, h - 45, UI_BG);
  tile_size = std::min((w - 30) / 4, (h - 49) / 2);
  int start_x = (w - (tile_size * 4 + 12)) / 2;
  int start_y = (h - 45 - (tile_size * 2 + 4)) / 2;
  for (int i = 0; i < GalleryNavigation::page_size; ++i) {
    tiles[i] = ui_block(ui->grid, start_x + (i % 4) * (tile_size + 4),
                        start_y + (i / 4) * (tile_size + 4), tile_size, tile_size, UI_PANEL);
    lv_obj_set_style_border_width(tiles[i], 2, 0);
    lv_obj_set_style_radius(tiles[i], 3, 0);
    tile_images[i] = lv_img_create(tiles[i]);
    tile_errors[i] = ui_label(tiles[i], LV_SYMBOL_WARNING, UI_MUTED);
    lv_obj_center(tile_errors[i]);
    lv_obj_add_flag(tile_errors[i], LV_OBJ_FLAG_HIDDEN);
  }
  ui_status_bar(ui->picture);
  ui->picture_counter = ui_footer(ui->picture, "");
  ui_attach_controls(ui->picture, ui->picture);
  lv_obj_add_event_cb(ui->picture, picture_event, LV_EVENT_KEY, nullptr);
  lv_obj_add_event_cb(ui->picture, gallery_deleted, LV_EVENT_DELETE, nullptr);
  cache_picture_list();
  loaded_index = loaded_page = -1;
  next_slot = 0;
  if (!nav.count) {
    lv_label_set_text(ui->message, SD_MMC.cardType() == CARD_NONE ? "Insert an SD card" : "No photos yet");
  } else lv_obj_add_flag(ui->message, LV_OBJ_FLAG_HIDDEN);
  update_chrome();
  ui->picture_timer = lv_timer_create(picture_tick, 20, nullptr);
}
