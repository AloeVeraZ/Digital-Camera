#include "camera_ui.h"
#include <atomic>
#include "Freenove_WS2812_Lib_for_ESP32.h" 

// Reference the global LED strip variable initialized in your main sketch
extern Freenove_ESP32_WS2812 strip; 

lv_img_dsc_t photo_show;          // apply an lvgl image variable
lvgl_camera_ui guider_camera_ui;  // camera ui structure

camera_fb_t *fb = NULL;              // data structure of camera frame buffer
TaskHandle_t cameraTaskHandle;       // camera thread task handle
static std::atomic<int> camera_task_flag{0};     // camera thread running flag
static std::atomic<int> camera_take_a_photo{0};  // take a photo flag
static std::atomic<bool> camera_stop_requested{false};
static std::atomic<int> capture_result{0};
static uint32_t feedback_until = 0;
static int fileCounter = -1;         // initialized from the SD card before first save
static lv_img_dsc_t preview_images[2];
static uint8_t *preview_buffers[2] = { NULL, NULL };
static size_t preview_buffer_size = 0;
static std::atomic<int> preview_ready_index{-1};
static std::atomic<int> preview_displayed_index{-1};

static void initialize_file_counter(void) {
  if (fileCounter >= 0) return;
  int highest = -1;
  std::list<File_Entry> files = list_dir(CAMERA_FOLDER, 0);
  for (const auto &entry : files) {
    if (entry.isDirectory) continue;
    int number = -1;
    if (sscanf(entry.name.c_str(), "photo%d", &number) == 1 && number > highest) {
      highest = number;
    }
  }
  fileCounter = highest + 1;
}

static void camera_flash_timer_cb(lv_timer_t *timer) {
  lv_obj_t *flash = (lv_obj_t *)timer->user_data;
  if (lv_obj_is_valid(flash)) lv_obj_add_flag(flash, LV_OBJ_FLAG_HIDDEN);
  guider_camera_ui.camera_flash_timer = NULL;
  lv_timer_del(timer);
}

static void show_shutter_flash(void) {
  if (!lv_obj_is_valid(guider_camera_ui.camera_flash)) return;
  lv_obj_clear_flag(guider_camera_ui.camera_flash, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(guider_camera_ui.camera_flash);
  if (guider_camera_ui.camera_flash_timer) {
    lv_timer_del(guider_camera_ui.camera_flash_timer);
  }
  guider_camera_ui.camera_flash_timer = lv_timer_create(camera_flash_timer_cb, 160,
                                                         guider_camera_ui.camera_flash);
  lv_timer_set_repeat_count(guider_camera_ui.camera_flash_timer, 1);
}

// Define a structure to hold the parameters
typedef struct
{
  uint32_t screen_width;
  uint32_t screen_height;
} camera_task_params_t;

// Create camera task thread
void create_camera_task(uint32_t screen_width, uint32_t screen_height) {
  if (camera_task_flag == 0) {
    camera_task_flag = 1;
    camera_stop_requested = false;
    camera_task_params_t *params = (camera_task_params_t *)malloc(sizeof(camera_task_params_t));
    if (params == NULL) {
      Serial.println("Failed to allocate memory for camera task parameters");
      camera_task_flag = 0;
      return;
    }
    params->screen_width = screen_width;
    params->screen_height = screen_height;
    if (xTaskCreate(loopTask_camera, "loopTask_camera", 8192, params, 1, &cameraTaskHandle) != pdPASS) {
      free(params);
      camera_task_flag = 0;
      capture_result = -1;
    }
  } else {
    Serial.println("loopTask_camera is running...");
  }
}

// Close the camera thread
void stop_camera_task(void) {
  camera_stop_requested = true;
  // The worker acknowledges shutdown before its handle can be reclaimed.
  while (camera_task_flag) vTaskDelay(1);
  camera_take_a_photo = 0;
  capture_result = 0;
}

// Rotate and crop the image
void rotate_and_crop_image(const uint8_t *input, int inputWidth, int inputHeight,
                           uint8_t *output, int outputWidth, int outputHeight, int angle) {
  int cropStartX, cropStartY;
  switch (angle) {
    case 0:
    case 180:
      cropStartX = (inputWidth - outputWidth) / 2;
      cropStartY = (inputHeight - outputHeight) / 2;
      break;
    case 90:
    case 270:
      cropStartX = (inputHeight - outputWidth) / 2;
      cropStartY = (inputWidth - outputHeight) / 2;
      break;
    default:
      cropStartX = (inputWidth - outputWidth) / 2;
      cropStartY = (inputHeight - outputHeight) / 2;
      break;
  }
  for (int y = 0; y < outputHeight; y++) {
    for (int x = 0; x < outputWidth; x++) {
      int rotatedX = cropStartX + x;
      int rotatedY = cropStartY + y;
      int inputX = rotatedX;
      int inputY = rotatedY;
      if (angle == 90) {
        inputX = rotatedY;
        inputY = inputHeight - 1 - rotatedX;
      } else if (angle == 180) {
        inputX = inputWidth - 1 - rotatedX;
        inputY = inputHeight - 1 - rotatedY;
      } else if (angle == 270) {
        inputX = inputWidth - 1 - rotatedY;
        inputY = rotatedX;
      }
      size_t source = (inputY * inputWidth + inputX) * 2;
      size_t destination = (y * outputWidth + x) * 2;
      output[destination] = input[source + 1];
      output[destination + 1] = input[source];
    }
  }
}

static void camera_preview_timer_cb(lv_timer_t *) {
  int result = capture_result.exchange(0);
  if (result) {
    lv_label_set_text(guider_camera_ui.camera_feedback, result > 0 ?
        "Saved to SD   " LV_SYMBOL_UP " Home" : "Save failed / check SD");
    lv_obj_center(guider_camera_ui.camera_feedback);
    feedback_until = millis() + 2000;
    storage_status_refresh();
  } else if (feedback_until && static_cast<int32_t>(millis() - feedback_until) >= 0) {
    lv_label_set_text(guider_camera_ui.camera_feedback, "OK Shutter   " LV_SYMBOL_UP " Home");
    lv_obj_center(guider_camera_ui.camera_feedback);
    feedback_until = 0;
  }
  int ready = preview_ready_index;
  if (ready < 0 || !lv_obj_is_valid(guider_camera_ui.camera_video)) return;
  lv_img_set_src(guider_camera_ui.camera_video, &preview_images[ready]);
  lv_obj_invalidate(guider_camera_ui.camera_video);
  preview_displayed_index = ready;
  preview_ready_index = -1;
}

// Camera thread
void loopTask_camera(void *pvParameters) {
  camera_task_params_t *params = (camera_task_params_t *)pvParameters;
  uint32_t screen_width = params->screen_width;
  uint32_t screen_height = params->screen_height;
  free(params);  
  Serial.println("loopTask_camera start...");
  while (!camera_stop_requested) {
    if (camera_take_a_photo == 0) {
      if (preview_ready_index >= 0) {
        vTaskDelay(1);
        continue;
      }
      camera_fb_t *fb = esp_camera_fb_get();
      if (!fb) {
        Serial.println("Camera capture failed");
        capture_result = -1;
        break;
      }
      int write_index = preview_displayed_index == 0 ? 1 : 0;
      uint8_t *buffer = preview_buffers[write_index];
      if (!buffer) {
        Serial.println("Camera preview buffer is unavailable");
        esp_camera_fb_return(fb);
        capture_result = -1;
        break;
      }
#ifdef FNK0102A_1P14_135x240_ST7789
      rotate_and_crop_image(fb->buf, fb->width, fb->height, buffer, screen_width, screen_height, 90);
#elif defined FNK0102B_3P5_320x480_ST7796
      rotate_and_crop_image(fb->buf, fb->width, fb->height, buffer, screen_width, screen_height, 0);
#endif
      esp_camera_fb_return(fb);
      preview_ready_index = write_index;
    } else {
      if (SD_MMC.cardType() == CARD_NONE) {
        capture_result = -1;
        camera_take_a_photo = 0;
        vTaskDelay(1);
        continue;
      }
      bool saved = false;
      camera_fb_t *fb = esp_camera_fb_get();
      if (!fb) {
        Serial.println("Camera capture failed");
        capture_result = -1;
        break;
      }

      // Shutter confirmation flash (Solid Purple)
      strip.setAllLedsColorData(255, 0, 255);
      strip.show();
      Serial.println("Saving photo, LED turned Purple...");

      char filename[255];
      initialize_file_counter();
      sensor_t *s = esp_camera_sensor_get();
      if (s->pixformat == PIXFORMAT_RGB565) {
        do {
          snprintf(filename, sizeof(filename), "%s/photo%d.bmp", CAMERA_FOLDER, fileCounter);
          if (SD_MMC.exists(filename)) ++fileCounter;
          else break;
        } while (true);
        Serial.printf("%s size is %d\r\n", filename, (screen_width * screen_height * 2));
        uint8_t *buffer = (uint8_t *)malloc(screen_width * screen_height * 2 * sizeof(uint8_t));
        if (!buffer) {
          Serial.println("Failed to allocate memory for rotated image");
          strip.setAllLedsColorData(0, 0, 0);
          strip.show();
          esp_camera_fb_return(fb);
          capture_result = -1;
          break;
        }
#ifdef FNK0102A_1P14_135x240_ST7789
        rotate_and_crop_image(fb->buf, fb->width, fb->height, buffer, screen_width, screen_height, 90);
#elif defined FNK0102B_3P5_320x480_ST7796
        rotate_and_crop_image(fb->buf, fb->width, fb->height, buffer, screen_width, screen_height, 0);
#endif
        saved = write_bmp(filename, buffer, (screen_height * screen_width * 2), screen_height, screen_width);
        free(buffer);
      } else if (s->pixformat == PIXFORMAT_JPEG) {
        do {
          snprintf(filename, sizeof(filename), "%s/photo%d.jpg", CAMERA_FOLDER, fileCounter);
          if (SD_MMC.exists(filename)) ++fileCounter;
          else break;
        } while (true);
        Serial.printf("%s size is %d\r\n", filename, fb->len);
        saved = write_jpg(filename, fb->buf, fb->len);
      }
      fileCounter++;

      strip.setAllLedsColorData(0, 0, 0);
      strip.show();
      Serial.println(saved ? "Photo saved to SD." : "Photo save failed.");

      esp_camera_fb_return(fb);
      camera_take_a_photo = 0;
      capture_result = saved ? 1 : -1;
    }
  }
  camera_task_flag = 0;
  vTaskDelete(nullptr);
}

// Initialize an lvgl image variable
void ui_set_photo_show(uint32_t screen_width, uint32_t screen_height) {
  lv_img_header_t header = {};
  header.always_zero = 0;
  header.w = screen_width;
  header.h = screen_height;
  header.cf = LV_IMG_CF_TRUE_COLOR;
  photo_show.header = header;
  photo_show.data_size = screen_width * screen_height * 2;
  photo_show.data = NULL;

  size_t required = screen_width * screen_height * 2;
  if (required > preview_buffer_size) {
    for (int i = 0; i < 2; ++i) {
      if (preview_buffers[i]) free(preview_buffers[i]);
      preview_buffers[i] = (uint8_t *)ps_malloc(required);
      if (!preview_buffers[i]) preview_buffers[i] = (uint8_t *)malloc(required);
    }
    preview_buffer_size = (preview_buffers[0] && preview_buffers[1]) ? required : 0;
  }
  for (int i = 0; i < 2; ++i) {
    preview_images[i].header = header;
    preview_images[i].data_size = required;
    preview_images[i].data = preview_buffers[i];
  }
  preview_ready_index = -1;
  preview_displayed_index = -1;
}

// Gesture event handler
static void camera_screen_gesture_event_handler(lv_event_t *event) {
  lv_event_code_t code = lv_event_get_code(event);
  if (code == LV_EVENT_KEY) {
    uint32_t c = lv_event_get_key(event);
    if (c == LV_KEY_ENTER) {
      if (camera_take_a_photo == 0) {
        if (!camera_task_flag) {
          lv_label_set_text(guider_camera_ui.camera_feedback, "Camera unavailable");
          lv_obj_center(guider_camera_ui.camera_feedback);
          return;
        }
        feedback_until = 0;
        lv_label_set_text(guider_camera_ui.camera_feedback, "Saving to SD...");
        lv_obj_center(guider_camera_ui.camera_feedback);
        show_shutter_flash();
        camera_take_a_photo = 1;
      }
    }
    // Listens for custom 'B' character (Physical UP button) to trigger return to home
    else if(c == 'B')
    {
      if (guider_camera_ui.camera_flash_timer) {
        lv_timer_del(guider_camera_ui.camera_flash_timer);
        guider_camera_ui.camera_flash_timer = NULL;
      }
      if (guider_camera_ui.camera_preview_timer) {
        lv_timer_del(guider_camera_ui.camera_preview_timer);
        guider_camera_ui.camera_preview_timer = NULL;
      }
      stop_camera_task();
      Serial.println("Jump to Main Screen!");
      lv_obj_t *old = guider_camera_ui.camera;
      setup_scr_main(&guider_main_ui);
      guider_camera_ui.camera = nullptr;
      lv_obj_del(old);
    }
  }
} 

// Parameter configuration function on the camera screen
void setup_scr_camera(lvgl_camera_ui *ui) {
  camera_build_view(ui);
  uint32_t screen_width = lv_disp_get_hor_res(nullptr);
  uint32_t screen_height = lv_disp_get_ver_res(nullptr);
  ui->camera_flash_timer = nullptr;
  feedback_until = 0;
  capture_result = 0;
  camera_take_a_photo = 0;
  ui_attach_controls(ui->camera, ui->camera);
  ui_set_photo_show(screen_width, screen_height);
  ui->camera_preview_timer = lv_timer_create(camera_preview_timer_cb, 30, nullptr);
  lv_obj_add_event_cb(ui->camera, camera_screen_gesture_event_handler, LV_EVENT_KEY, nullptr);
  create_camera_task(screen_width, screen_height);
}

// Camera initialization function
void camera_init(int state) {
  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.grab_mode = CAMERA_GRAB_LATEST;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 2;
  if (state == 0) {
#ifdef FNK0102A_1P14_135x240_ST7789
    config.frame_size = FRAMESIZE_240X240;
#elif defined FNK0102B_3P5_320x480_ST7796
    config.frame_size = FRAMESIZE_HVGA;
#endif
    config.pixel_format = PIXFORMAT_RGB565;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.pixel_format = PIXFORMAT_JPEG;
  }
  esp_camera_deinit();
  esp_camera_return_all();
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera initialization failed, error code 0x%x", err);
    return;
  }
  sensor_t *s = esp_camera_sensor_get();
  uint8_t pid = s->id.PID;
#ifdef FNK0102A_1P14_135x240_ST7789
  if(pid == 0x45)
  {
    s->set_hmirror(s, 0);
    vTaskDelay(500);
    s->set_vflip(s, 0);       
  }else if(pid == 0x26)
  {
    s->set_hmirror(s, 1);
    s->set_vflip(s, 1);       
  }else if(pid == 0x9B)
  {
    s->set_hmirror(s, 1);
    vTaskDelay(500);
    s->set_vflip(s, 1);       
  }
  else{
    s->set_hmirror(s, 1);
    s->set_vflip(s, 0);       
  }
#elif defined FNK0102B_3P5_320x480_ST7796
  if(pid == 0x45)
  {
    s->set_hmirror(s, 1);
    vTaskDelay(500);
    s->set_vflip(s, 1);       
  }else if(pid == 0x26)
  {
    s->set_hmirror(s, 0);
    vTaskDelay(500);
    s->set_vflip(s, 0);       
  }else if(pid == 0x9B)
  {
    s->set_hmirror(s, 0);
    vTaskDelay(500);
    s->set_vflip(s, 0);       
  }
  else{
    s->set_hmirror(s, 0);
    s->set_vflip(s, 1);       
  }
#endif
  // Balanced automatic imaging with the sensor's cleanup stages enabled.
  // The old -3 exposure compensation heavily underexposed dark scenes and
  // forced visible grain in the preview.
  s->set_brightness(s, 0);
  s->set_contrast(s, 1);
  s->set_saturation(s, 0);
  s->set_ae_level(s, 0);
  s->set_exposure_ctrl(s, 1);
  s->set_gain_ctrl(s, 1);
  s->set_whitebal(s, 1);
  s->set_awb_gain(s, 1);
  s->set_denoise(s, 1);
  s->set_bpc(s, 1);
  s->set_wpc(s, 1);
  s->set_raw_gma(s, 1);
  s->set_lenc(s, 1);
}
