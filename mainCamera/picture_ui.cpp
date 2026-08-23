#include "picture_ui.h"
#include <algorithm>
#include <vector>
#include "esp_heap_caps.h"

lvgl_picture_ui guider_picture_ui;
static int picture_index = 0;
static int current_picture_index = -1;
static int total_pictures = 0;
static std::vector<String> picture_files;
static char current_picture_path[192];

struct PictureCacheSlot {
  uint8_t *pixels;
  size_t capacity;
  lv_img_dsc_t image;
  String file_name;
};
static PictureCacheSlot picture_cache[2] = {};
static int next_cache_slot = 0;

static uint32_t read_le32(const uint8_t *bytes) {
  return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
         ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

static bool load_bmp_fast(const char *name, int slot_index) {
  snprintf(current_picture_path, sizeof(current_picture_path), "%s/%s", PICTURE_FOLDER, name);
  fs::File file = SD_MMC.open(current_picture_path, FILE_READ);
  if (!file) return false;

  uint8_t header[70];
  if (file.read(header, sizeof(header)) != sizeof(header) || header[0] != 'B' ||
      header[1] != 'M' || header[28] != 16 || header[29] != 0) {
    file.close();
    return false;
  }
  uint32_t pixel_offset = read_le32(header + 10);
  uint32_t width = read_le32(header + 18);
  int32_t signed_height = (int32_t)read_le32(header + 22);
  bool bottom_up = signed_height > 0;
  uint32_t height = bottom_up ? signed_height : -signed_height;
  if (!width || !height || width > 480 || height > 480) {
    file.close();
    return false;
  }

  size_t bytes_needed = width * height * 2;
  PictureCacheSlot &slot = picture_cache[slot_index];
  if (slot.capacity < bytes_needed) {
    if (slot.pixels) free(slot.pixels);
    slot.pixels = (uint8_t *)heap_caps_malloc(bytes_needed, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!slot.pixels) slot.pixels = (uint8_t *)malloc(bytes_needed);
    if (!slot.pixels) {
      slot.capacity = 0;
      file.close();
      return false;
    }
    slot.capacity = bytes_needed;
  }

  size_t raw_stride = width * 2;
  size_t padded_stride = (raw_stride + 3) & ~((size_t)3);
  size_t available = file.size() > pixel_offset ? file.size() - pixel_offset : 0;
  size_t source_stride = available >= padded_stride * height ? padded_stride : raw_stride;
  file.seek(pixel_offset, fs::SeekSet);
  for (uint32_t source_row = 0; source_row < height; ++source_row) {
    uint32_t destination_row = bottom_up ? height - 1 - source_row : source_row;
    if (file.read(slot.pixels + destination_row * raw_stride, raw_stride) != raw_stride) {
      file.close();
      return false;
    }
    if (source_stride > raw_stride) {
      file.seek(file.position() + source_stride - raw_stride, fs::SeekSet);
    }
  }
  file.close();

  slot.image.header.always_zero = 0;
  slot.image.header.w = width;
  slot.image.header.h = height;
  slot.image.header.cf = LV_IMG_CF_TRUE_COLOR;
  slot.image.data_size = bytes_needed;
  slot.image.data = slot.pixels;
  slot.file_name = name;
  return true;
}

static int photo_number(const String &name) {
  int number = -1;
  return sscanf(name.c_str(), "photo%d", &number) == 1 ? number : -1;
}

static bool is_picture_file(const String &name) {
  String lower = name;
  lower.toLowerCase();
  return lower.endsWith(".bmp") || lower.endsWith(".jpg") || lower.endsWith(".jpeg");
}

static void cache_picture_list(void) {
  picture_files.clear();
  std::list<File_Entry> files = list_dir(PICTURE_FOLDER, 0);
  for (const auto &entry : files) {
    if (!entry.isDirectory && is_picture_file(entry.name)) picture_files.push_back(entry.name);
  }

  // Newest first. Camera-generated photo numbers provide a stable order even
  // when FAT directory iteration changes after a reboot.
  std::sort(picture_files.begin(), picture_files.end(), [](const String &a, const String &b) {
    int a_number = photo_number(a);
    int b_number = photo_number(b);
    if (a_number >= 0 && b_number >= 0) return a_number > b_number;
    if (a_number >= 0) return true;
    if (b_number >= 0) return false;
    return a.compareTo(b) > 0;
  });
  total_pictures = (int)picture_files.size();
}

static void update_navigation_chrome(void) {
  if (!lv_obj_is_valid(guider_picture_ui.picture_counter)) return;
  if (total_pictures <= 0) {
    lv_obj_add_flag(guider_picture_ui.picture_left, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(guider_picture_ui.picture_right, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(guider_picture_ui.picture_counter, LV_OBJ_FLAG_HIDDEN);
    return;
  }

  lv_obj_clear_flag(guider_picture_ui.picture_left, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(guider_picture_ui.picture_right, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(guider_picture_ui.picture_counter, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_style_opa(guider_picture_ui.picture_left,
                       picture_index > 0 ? LV_OPA_80 : LV_OPA_20, 0);
  lv_obj_set_style_opa(guider_picture_ui.picture_right,
                       picture_index < total_pictures - 1 ? LV_OPA_80 : LV_OPA_20, 0);
  if (picture_index == 0) {
    lv_label_set_text_fmt(guider_picture_ui.picture_counter, "1 / %d  LATEST", total_pictures);
  } else {
    lv_label_set_text_fmt(guider_picture_ui.picture_counter, "%d / %d", picture_index + 1,
                          total_pictures);
  }
  lv_obj_align(guider_picture_ui.picture_counter, LV_ALIGN_BOTTOM_MID, 0, -5);
}

void picture_timer_event_handler(lv_timer_t *timer) {
  if (total_pictures > 0 && picture_index >= 0 && picture_index < total_pictures &&
      picture_index != current_picture_index) {
    picture_display(picture_files[picture_index].c_str());
    current_picture_index = picture_index;
  }
  update_navigation_chrome();
  guider_picture_ui.picture_timer = NULL;
  lv_timer_del(timer);
}

static void schedule_picture_load(void) {
  if (!guider_picture_ui.picture_timer) {
    guider_picture_ui.picture_timer = lv_timer_create(picture_timer_event_handler, 1, NULL);
    lv_timer_set_repeat_count(guider_picture_ui.picture_timer, 1);
  }
}

static void picture_event_handler(lv_event_t *event) {
  if (lv_event_get_code(event) != LV_EVENT_KEY) return;
  uint32_t key = lv_event_get_key(event);

  if (key == 'B') {
    if (guider_picture_ui.picture_timer) {
      lv_timer_del(guider_picture_ui.picture_timer);
      guider_picture_ui.picture_timer = NULL;
    }
    Serial.println("Jump to Main Screen!");
    if (!lv_obj_is_valid(guider_main_ui.main)) setup_scr_main(&guider_main_ui);
    lv_scr_load(guider_main_ui.main);
    lv_obj_del(guider_picture_ui.picture);
    picture_files.clear();
    return;
  }

  // Left moves toward newer photos; right moves toward older photos.
  if (key == LV_KEY_LEFT && picture_index > 0) {
    --picture_index;
    update_navigation_chrome();
    schedule_picture_load();
  } else if (key == LV_KEY_RIGHT && picture_index < total_pictures - 1) {
    ++picture_index;
    update_navigation_chrome();
    schedule_picture_load();
  }
}

static lv_obj_t *create_arrow(lv_obj_t *parent, const char *symbol, lv_align_t align) {
  lv_obj_t *arrow = lv_label_create(parent);
  lv_label_set_text(arrow, symbol);
  lv_obj_set_size(arrow, 30, 30);
  lv_obj_set_style_radius(arrow, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(arrow, lv_color_hex(0x17202A), 0);
  lv_obj_set_style_bg_opa(arrow, LV_OPA_70, 0);
  lv_obj_set_style_text_color(arrow, lv_color_white(), 0);
  lv_obj_set_style_text_align(arrow, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_pad_top(arrow, 7, 0);
  lv_obj_align(arrow, align, align == LV_ALIGN_LEFT_MID ? 5 : -5, 0);
  return arrow;
}

void setup_scr_picture(lvgl_picture_ui *ui) {
  ui->picture = lv_obj_create(NULL);
  lv_coord_t screen_width = lv_disp_get_hor_res(NULL);
  lv_coord_t screen_height = lv_disp_get_ver_res(NULL);
  lv_obj_set_size(ui->picture, screen_width, screen_height);
  lv_obj_clear_flag(ui->picture, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(ui->picture, lv_color_hex(0x111820), 0);

  ui->picture_show = lv_img_create(ui->picture);
  lv_obj_set_size(ui->picture_show, screen_width, screen_height);
  lv_obj_center(ui->picture_show);

  ui->picture_left = create_arrow(ui->picture, LV_SYMBOL_LEFT, LV_ALIGN_LEFT_MID);
  ui->picture_right = create_arrow(ui->picture, LV_SYMBOL_RIGHT, LV_ALIGN_RIGHT_MID);
  ui->picture_counter = lv_label_create(ui->picture);
  lv_obj_set_style_bg_color(ui->picture_counter, lv_color_hex(0x17202A), 0);
  lv_obj_set_style_bg_opa(ui->picture_counter, LV_OPA_70, 0);
  lv_obj_set_style_text_color(ui->picture_counter, lv_color_white(), 0);
  lv_obj_set_style_pad_hor(ui->picture_counter, 8, 0);
  lv_obj_set_style_pad_ver(ui->picture_counter, 3, 0);
  lv_obj_set_style_radius(ui->picture_counter, 8, 0);
  battery_status_attach(ui->picture);

  lv_group_t *group = lv_group_create();
  lv_group_add_obj(group, ui->picture);
  lv_group_set_editing(group, true);
  lv_indev_set_group(indev_keypad, group);
  lv_obj_add_event_cb(ui->picture, picture_event_handler, LV_EVENT_KEY, NULL);

  cache_picture_list();
  picture_index = 0;
  current_picture_index = -1;
  ui->picture_timer = NULL;
  lv_img_cache_invalidate_src(NULL);

  if (total_pictures > 0) {
    update_navigation_chrome();
    schedule_picture_load();
  } else {
    lv_img_set_src(ui->picture_show, LV_SYMBOL_DUMMY "  No images on the SD card");
    lv_obj_center(ui->picture_show);
    update_navigation_chrome();
  }
}

void picture_display(const char *name) {
  if (name && name[0]) {
    String lower = name;
    lower.toLowerCase();
    if (lower.endsWith(".bmp")) {
      for (int i = 0; i < 2; ++i) {
        if (picture_cache[i].pixels && picture_cache[i].file_name == name) {
          lv_img_set_src(guider_picture_ui.picture_show, &picture_cache[i].image);
          return;
        }
      }
      int slot = next_cache_slot;
      next_cache_slot = (next_cache_slot + 1) % 2;
      if (load_bmp_fast(name, slot)) {
        lv_img_set_src(guider_picture_ui.picture_show, &picture_cache[slot].image);
        return;
      }
    }
    snprintf(current_picture_path, sizeof(current_picture_path), "S:%s/%s", PICTURE_FOLDER, name);
    // The path buffer is persistent; LVGL keeps this pointer after lv_img_set_src returns.
    lv_img_set_src(guider_picture_ui.picture_show, current_picture_path);
  } else {
    lv_img_set_src(guider_picture_ui.picture_show, LV_SYMBOL_DUMMY "  No images on the SD card");
  }
}
