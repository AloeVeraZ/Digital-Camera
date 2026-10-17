#include "public.h"
#include "diagnostics.h"

#define SD_MMC_CMD 38         
#define SD_MMC_CLK 39         
#define SD_MMC_D0 40          

Display screen;  

#ifdef FNK0102B_3P5_320x480_ST7796
#define STATUS_LED_COUNT 4
#else
#define STATUS_LED_COUNT 1
#endif
Freenove_ESP32_WS2812 strip = Freenove_ESP32_WS2812(STATUS_LED_COUNT, 48, 0, TYPE_GRB);

static void mute_speaker_hardware(void) {
  // The MAX98357 input pins otherwise float during boot and can produce a ring.
  const uint8_t audio_pins[] = { 42, 41, 1 }; // BCLK, LRCLK, DATA
  for (uint8_t pin : audio_pins) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }
}

void setup() {
  mute_speaker_hardware();
  Serial.begin(115200);
  // Start on battery even when no serial monitor is attached.
  
  // Hardware setups
  pinMode(20, INPUT);  // Battery sense pin used by the Freenove battery example
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  camera_init(0);
  sdmmc_init(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);

  // Initialize the WS2812 status LED light modules
  strip.begin();
  strip.setBrightness(100);
  strip.setAllLedsColorData(0, 0, 0); // Ensure it starts completely OFF
  strip.show();

  // Set up storage folder directory
  create_dir(CAMERA_FOLDER);

  /*** Initialize the screen ***/
  screen.init(TFT_DIRECTION);  
  battery_status_init();

  // Load the menu screen
  setup_scr_main(&guider_main_ui);
  diagnostics_init();
}

void loop() {
  screen.routine();
  diagnostics_poll();
  delay(5);
}
