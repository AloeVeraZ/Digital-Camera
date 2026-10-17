#include "public.h"
#include "diagnostics.h"

static const char *LOG_PATH = "/battery-test.csv";
static const char *ARM_PATH = "/battery-test.enabled";
static constexpr uint32_t LOG_INTERVAL_MS = 5000;
static constexpr uint32_t MAX_SESSION_MS = 20UL * 60UL * 1000UL;
static constexpr size_t MAX_LOG_BYTES = 32768;
static bool recording = false;
static uint32_t last_log_ms = 0, started_ms = 0, last_sequence = 0;
static uint32_t boot_id = 0;
static char command[80];
static unsigned command_length = 0;

static bool append_text(const char *text) {
  fs::File file = SD_MMC.open(LOG_PATH, FILE_APPEND);
  if (!file) return false;
  size_t length = strlen(text);
  bool ok = file.write(reinterpret_cast<const uint8_t *>(text), length) == length;
  file.close();
  return ok;
}

static void stop_test() {
  recording = false;
  SD_MMC.remove(ARM_PATH);
  Serial.println("BATTERY TEST stopped. Log retained at /battery-test.csv");
}

static const char *screen_mode() {
  lv_obj_t *screen = lv_scr_act();
  if (screen == guider_camera_ui.camera) return "viewfinder";
  if (screen == guider_picture_ui.picture) return "playback";
  return "home";
}

static void log_sample() {
  BatteryReading r = battery_status_reading();
  if (!r.sequence || r.sequence == last_sequence) return;
  last_sequence = r.sequence;
  fs::File file = SD_MMC.open(LOG_PATH, FILE_READ);
  size_t size = file ? file.size() : 0;
  file.close();
  if (size >= MAX_LOG_BYTES || millis() - started_ms >= MAX_SESSION_MS) {
    append_text("# AUTO_STOP bounded test complete\n");
    stop_test();
    return;
  }
  char row[160];
  snprintf(row, sizeof(row), "%08lx,%lu,%d,%d,%d,%d,%d,%s\n",
           static_cast<unsigned long>(boot_id), static_cast<unsigned long>(r.sampled_ms),
           r.adc_mv, r.cell_mv, r.filtered_mv, r.percent, r.valid ? 1 : 0, screen_mode());
  if (!append_text(row)) {
    recording = false;
    Serial.println("BATTERY TEST log write failed; check SD. No new samples are being saved.");
  }
}

static void start_test() {
  if (SD_MMC.cardType() == CARD_NONE) {
    Serial.println("BATTERY TEST cannot start: no SD card.");
    return;
  }
  // Explicit start creates a fresh diagnostic log, never changes /picture.
  fs::File file = SD_MMC.open(LOG_PATH, FILE_WRITE);
  if (!file) { Serial.println("BATTERY TEST cannot create log."); return; }
  const char *header = "boot_id,uptime_ms,adc_mv,cell_mv,filtered_mv,estimate_percent,sample_valid,screen\n";
  size_t length = strlen(header);
  bool ok = file.write(reinterpret_cast<const uint8_t *>(header), length) == length;
  file.close();
  fs::File arm = SD_MMC.open(ARM_PATH, FILE_WRITE);
  const uint8_t enabled = 1;
  ok = ok && arm && arm.write(&enabled, 1) == 1;
  arm.close();
  if (!ok) { stop_test(); Serial.println("BATTERY TEST start failed."); return; }
  recording = true;
  started_ms = last_log_ms = millis();
  last_sequence = 0;
  log_sample();
  Serial.println("BATTERY TEST armed: logs every 5 seconds, resumes after reboot; stops after 20 minutes per boot or 32 KiB.");
}

static void storage_report() {
  uint64_t total = SD_MMC.cardType() == CARD_NONE ? 0 : SD_MMC.totalBytes();
  uint64_t used = total ? SD_MMC.usedBytes() : 0;
  uint64_t photos = 0;
  unsigned count = 0;
  if (total) for (const auto &entry : list_dir(PICTURE_FOLDER, 0)) {
    String lower = entry.name;
    lower.toLowerCase();
    if (!entry.isDirectory && (lower.endsWith(".bmp") || lower.endsWith(".jpg") || lower.endsWith(".jpeg"))) {
      photos += entry.size; ++count;
    }
  }
  Serial.printf("STORAGE SD: total_bytes=%llu used_bytes=%llu free_bytes=%llu\n", total, used, total > used ? total - used : 0);
  Serial.printf("STORAGE PHOTOS: folder=/picture files=%u logical_bytes=%llu\n", count, photos);
  Serial.printf("STORAGE ESP: flash_bytes=%lu sketch_bytes=%lu (onboard firmware, separate from SD)\n",
                static_cast<unsigned long>(ESP.getFlashChipSize()), static_cast<unsigned long>(ESP.getSketchSize()));
}

static void dump_test() {
  fs::File file = SD_MMC.open(LOG_PATH, FILE_READ);
  Serial.println("BATTERY_LOG_BEGIN");
  if (file) {
    uint8_t buffer[128];
    size_t length;
    while ((length = file.read(buffer, sizeof(buffer))) > 0) Serial.write(buffer, length);
    file.close();
  } else Serial.println("# No battery test log");
  Serial.println("BATTERY_LOG_END");
}

static void execute_command() {
  if (!strcmp(command, "battery start")) start_test();
  else if (!strcmp(command, "battery stop")) stop_test();
  else if (!strcmp(command, "battery dump")) dump_test();
  else if (!strcmp(command, "battery status")) {
    Serial.printf("BATTERY TEST %s; log=/battery-test.csv\n", recording ? "recording" : "stopped");
  } else if (!strncmp(command, "battery mark ", 13)) {
    if (recording) {
      char row[160];
      snprintf(row, sizeof(row), "# MARK %08lx %lu %s\n", static_cast<unsigned long>(boot_id),
               static_cast<unsigned long>(millis()), command + 13);
      append_text(row);
      Serial.println(row);
    }
  } else if (!strcmp(command, "storage")) storage_report();
  else Serial.println("Commands: storage; battery start|stop|status|dump|mark <note>");
}

void diagnostics_init(void) {
  boot_id = esp_random();
  recording = SD_MMC.cardType() != CARD_NONE && SD_MMC.exists(ARM_PATH);
  started_ms = last_log_ms = millis();
  if (recording) {
    char row[80];
    snprintf(row, sizeof(row), "# BOOT %08lx\n", static_cast<unsigned long>(boot_id));
    if (!append_text(row)) recording = false;
  }
  Serial.printf("Diagnostics ready. Battery test %s. Send 'storage' for exact SD/photo/ESP byte counts.\n",
                recording ? "resumed" : "stopped");
}

void diagnostics_poll(void) {
  // Bounded input processing keeps a serial sender from monopolizing the UI.
  for (int i = 0; i < 80 && Serial.available(); ++i) {
    int c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (command_length) { command[command_length] = 0; execute_command(); command_length = 0; }
    } else if (c >= 32 && c < 127 && command_length < sizeof(command) - 1) {
      command[command_length++] = static_cast<char>(c);
    }
  }
  if (recording && millis() - last_log_ms >= LOG_INTERVAL_MS) {
    last_log_ms = millis();
    log_sample();
  }
}
