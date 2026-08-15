#include "battery_status.h"
#include "Arduino.h"
#include "display.h"
#include <Preferences.h>

static const uint8_t BATTERY_PIN = 20;
static const int BATTERY_MIN_MV = 3200;
static const int BATTERY_MAX_MV = 4200;
static const uint32_t SAMPLE_PERIOD_MS = 10000;

static lv_timer_t *sample_timer = NULL;
static lv_obj_t *widget = NULL;
static lv_obj_t *battery_fill = NULL;
static lv_obj_t *battery_label = NULL;
static lv_obj_t *charge_icon = NULL;
static int filtered_mv = 0;
static int previous_raw_mv = 0;
static int displayed_percent = -1;
static int charge_score = 0;
static bool charging = false;
static uint32_t last_percent_change = 0;
static uint32_t last_saved = 0;
static bool have_valid_reading = false;
static Preferences battery_preferences;

static int read_battery_millivolts(bool &usb_detected) {
  int samples[15];
  int count = 0;
  int usb_high_count = 0;
  pinMode(BATTERY_PIN, INPUT);
  for (int i = 0; i < 15; ++i) {
    int value = (int)(analogReadMilliVolts(BATTERY_PIN) * 2.5f) - 3300;
    // The Freenove example treats <=3.2 V as invalid. Accepting those samples
    // previously converted a Type-C-disturbed ADC reading into a false 0%.
    if (value > BATTERY_MIN_MV && value < 4300) samples[count++] = value;
    else if (value >= 4300 || value <= BATTERY_MIN_MV) ++usb_high_count;
    delay(1);
  }
  // GPIO20 is also LCD reset. It must remain INPUT after the one-time reset
  // pulse; driving it high here biases the following ADC measurements.
  pinMode(LCD_RESET_BATTERY_PIN, INPUT);
  // On this board a Type-C connection can make GPIO 20 entirely invalid as
  // an ADC source. An all-invalid sample window is therefore charger present.
  usb_detected = usb_high_count >= 8 || count == 0;
  if (!count) return 0;

  for (int i = 1; i < count; ++i) {
    int value = samples[i];
    int j = i - 1;
    while (j >= 0 && samples[j] > value) {
      samples[j + 1] = samples[j];
      --j;
    }
    samples[j + 1] = value;
  }
  return samples[count / 2];
}

// Piecewise Li-ion discharge curve. A linear 3.2-4.2 V mapping exaggerates
// normal voltage sag around the long middle plateau.
static int voltage_to_percent(int mv) {
  static const int volts[] = { 3200, 3400, 3500, 3600, 3680, 3740, 3790,
                               3840, 3900, 3970, 4050, 4120, 4200 };
  static const int percents[] = { 0, 5, 10, 18, 28, 38, 50,
                                  62, 72, 82, 90, 96, 100 };
  if (mv <= volts[0]) return 0;
  if (mv >= volts[12]) return 100;
  for (int i = 1; i < 13; ++i) {
    if (mv <= volts[i]) {
      int percent = percents[i - 1] + (mv - volts[i - 1]) *
                    (percents[i] - percents[i - 1]) / (volts[i] - volts[i - 1]);
      return max(1, percent);
    }
  }
  return 100;
}

static void update_widget(void) {
  if (!widget || !lv_obj_is_valid(widget)) return;
  if (displayed_percent < 0) {
    lv_label_set_text(battery_label, "--");
    lv_obj_set_width(battery_fill, 1);
  } else {
    lv_label_set_text_fmt(battery_label, "%d", displayed_percent);
    lv_obj_set_width(battery_fill, 1 + displayed_percent * 31 / 100);
  }
  lv_obj_set_style_bg_color(battery_fill,
      charging ? lv_color_hex(0x34C759) :
      (displayed_percent >= 0 && displayed_percent < 20 ? lv_color_hex(0xFF453A)
                                                        : lv_color_hex(0x3B82F6)), 0);
  if (charging) lv_obj_clear_flag(charge_icon, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_add_flag(charge_icon, LV_OBJ_FLAG_HIDDEN);
}

static void sample_battery(lv_timer_t *) {
  int percent_before_sample = displayed_percent;
  bool charging_before_sample = charging;
  bool usb_detected = false;
  int raw_mv = read_battery_millivolts(usb_detected);
  if (usb_detected) {
    charging = true;
    charge_score = 6;
  }
  if (!raw_mv) {
    if (charging != charging_before_sample) {
      battery_preferences.putBool("charging", charging);
    }
    update_widget();
    Serial.printf("Battery: USB=%s, retaining last valid %d%%\n",
                  usb_detected ? "connected" : "unknown", displayed_percent);
    return;
  }

  if (!have_valid_reading) {
    filtered_mv = raw_mv;
    displayed_percent = voltage_to_percent(raw_mv);
    have_valid_reading = true;
    last_percent_change = millis();
  } else if (!filtered_mv) filtered_mv = raw_mv;
  else filtered_mv = (filtered_mv * 7 + raw_mv) / 8;

  if (previous_raw_mv && !usb_detected) {
    int delta = raw_mv - previous_raw_mv;
    if (!charging && delta >= 15) charge_score = 2;
    else if (delta >= 5) charge_score = min(charge_score + 1, 6);
    else if (charging && delta <= -40) charge_score = -3;
    else if (delta <= -20) charge_score = max(charge_score - 2, -6);
    else if (!charging && charge_score > 0) --charge_score;
    else if (!charging && charge_score < 0) ++charge_score;

    if (charge_score >= 2) {
      charging = true;
    } else if (charge_score <= -3) {
      charging = false;
    }
  }
  previous_raw_mv = raw_mv;

  int target = voltage_to_percent(filtered_mv);
  if (displayed_percent < 0) displayed_percent = target;
  else if (target <= displayed_percent - 3 && millis() - last_percent_change >= 300000UL) {
    // Real battery capacity cannot fall several percent in seconds. Camera and
    // backlight load cause temporary voltage sag, so downward changes are slow.
    --displayed_percent;
    last_percent_change = millis();
  } else if (target >= displayed_percent + 3 && millis() - last_percent_change >= 60000UL) {
    ++displayed_percent;
    last_percent_change = millis();
  }

  // A USB power transition can reset the board. Save every visible percentage
  // and charging-state change so a reboot never rolls 10% back to stale 8%.
  if (displayed_percent != percent_before_sample) {
    battery_preferences.putInt("percent", displayed_percent);
    battery_preferences.putInt("millivolts", filtered_mv);
    last_saved = millis();
  }
  if (charging != charging_before_sample) {
    battery_preferences.putBool("charging", charging);
  }

  if (millis() - last_saved >= 300000UL || last_saved == 0) {
    battery_preferences.putInt("percent", displayed_percent);
    battery_preferences.putInt("millivolts", filtered_mv);
    battery_preferences.putBool("charging", charging);
    last_saved = millis();
  }
  update_widget();
  Serial.printf("Battery: raw=%dmV filtered=%dmV shown=%d%% charging=%s\n",
                raw_mv, filtered_mv, displayed_percent, charging ? "yes" : "no");
}

void battery_status_init(void) {
  if (sample_timer) return;
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  battery_preferences.begin("battery", false);
  int saved_percent = battery_preferences.getInt("percent", -1);
  // Discard the false 0% value written by older firmware.
  have_valid_reading = battery_preferences.isKey("percent") && saved_percent > 1 &&
                       saved_percent <= 100;
  displayed_percent = have_valid_reading ? saved_percent : 50;
  filtered_mv = have_valid_reading ? battery_preferences.getInt("millivolts", 0) : 0;
  charging = battery_preferences.getBool("charging", false);
  charge_score = charging ? 2 : 0;
  sample_battery(NULL);
  sample_timer = lv_timer_create(sample_battery, SAMPLE_PERIOD_MS, NULL);
}

void battery_status_attach(lv_obj_t *parent) {
  widget = lv_obj_create(parent);
  lv_obj_remove_style_all(widget);
  lv_obj_set_size(widget, 58, 25);
  lv_obj_align(widget, LV_ALIGN_TOP_RIGHT, -3, 3);
  lv_obj_set_style_radius(widget, 10, 0);
  lv_obj_set_style_bg_color(widget, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(widget, LV_OPA_50, 0);
  lv_obj_clear_flag(widget, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

  charge_icon = lv_label_create(widget);
  lv_label_set_text(charge_icon, LV_SYMBOL_CHARGE);
  lv_obj_set_style_text_color(charge_icon, lv_color_hex(0x34C759), 0);
  lv_obj_align(charge_icon, LV_ALIGN_LEFT_MID, 4, 0);

  lv_obj_t *outline = lv_obj_create(widget);
  lv_obj_remove_style_all(outline);
  lv_obj_set_size(outline, 39, 19);
  lv_obj_align(outline, LV_ALIGN_RIGHT_MID, -4, 0);
  lv_obj_set_style_radius(outline, 4, 0);
  lv_obj_set_style_border_width(outline, 2, 0);
  lv_obj_set_style_border_color(outline, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(outline, LV_OPA_TRANSP, 0);
  lv_obj_clear_flag(outline, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

  battery_fill = lv_obj_create(outline);
  lv_obj_remove_style_all(battery_fill);
  lv_obj_set_height(battery_fill, 13);
  lv_obj_align(battery_fill, LV_ALIGN_LEFT_MID, 2, 0);
  lv_obj_set_style_radius(battery_fill, 2, 0);
  lv_obj_set_style_bg_opa(battery_fill, LV_OPA_COVER, 0);

  battery_label = lv_label_create(outline);
  lv_obj_set_style_text_color(battery_label, lv_color_white(), 0);
  lv_obj_set_style_text_font(battery_label, LV_FONT_DEFAULT, 0);
  lv_obj_center(battery_label);

  lv_obj_t *cap = lv_obj_create(widget);
  lv_obj_remove_style_all(cap);
  lv_obj_set_size(cap, 2, 7);
  lv_obj_align_to(cap, outline, LV_ALIGN_OUT_RIGHT_MID, 1, 0);
  lv_obj_set_style_bg_color(cap, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(cap, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(cap, 1, 0);
  update_widget();
}

int battery_status_percent(void) { return displayed_percent; }
bool battery_status_is_charging(void) { return charging; }
