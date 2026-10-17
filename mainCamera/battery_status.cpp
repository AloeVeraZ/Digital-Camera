#include "battery_status.h"
#include "battery_math.h"
#include "Arduino.h"
#include "display.h"
#include "ui_theme.h"
#include <algorithm>

static lv_timer_t *sample_timer = nullptr;
static lv_obj_t *widget = nullptr, *battery_icon = nullptr, *battery_label = nullptr;
static int displayed_percent = -1, filtered_mv = 0;
static int window_samples[21];
static unsigned window_count = 0, invalid_windows = 0;
static int edge_baseline = 0;
static uint32_t settling_since = 0;
static bool settling = false;
static BatteryReading latest_reading = {};

static void update_widget() {
  if (!widget) return;
  if (displayed_percent < 0) lv_label_set_text(battery_label, "--%");
  else lv_label_set_text_fmt(battery_label, "~%d%%", displayed_percent);
  const char *icon = displayed_percent < 0 ? LV_SYMBOL_BATTERY_EMPTY :
      displayed_percent < 15 ? LV_SYMBOL_BATTERY_1 :
      displayed_percent < 50 ? LV_SYMBOL_BATTERY_2 :
      displayed_percent < 85 ? LV_SYMBOL_BATTERY_3 : LV_SYMBOL_BATTERY_FULL;
  lv_label_set_text(battery_icon, icon);
  lv_obj_set_style_text_color(battery_icon, lv_color_hex(
      displayed_percent < 0 ? UI_MUTED : displayed_percent < 15 ? 0xF28B82 :
      displayed_percent < 30 ? UI_ACCENT : 0xA8CBA4), 0);
}

// One calibrated ADC read per tick. No blocking sampling delays in the UI task.
static void sample_battery(lv_timer_t *) {
  int adc_mv = analogReadMilliVolts(LCD_RESET_BATTERY_PIN);
  window_samples[window_count++] = adc_mv;
  if (window_count < 21) return;
  window_count = 0;
  std::sort(window_samples, window_samples + 21);
  int adc = 0;
  for (int i = 4; i < 17; ++i) adc += window_samples[i];
  adc /= 13;
  int raw_mv = battery_cell_mv(adc);
  latest_reading.sampled_ms = millis();
  ++latest_reading.sequence;
  latest_reading.adc_mv = adc;
  latest_reading.cell_mv = raw_mv;
  latest_reading.valid = false;
  // Bad ADC data must not freeze an old plausible-looking percentage.
  if (raw_mv < 2500 || raw_mv > 4350 ||
      window_samples[16] - window_samples[4] > 100) {
    if (++invalid_windows >= 3) {
      displayed_percent = -1;
      filtered_mv = 0;
      settling = false;
    }
    latest_reading.filtered_mv = filtered_mv;
    latest_reading.percent = -1;
    update_widget();
    Serial.printf("Battery: invalid ADC=%dmV cell=%dmV spread=%dmV\n", adc,
                  raw_mv, window_samples[16] - window_samples[4]);
    return;
  }
  invalid_windows = 0;
  uint32_t now = millis();
  if (!filtered_mv) filtered_mv = raw_mv;
  // Let a power/load transition settle. Never infer USB or charging from it.
  if (abs(raw_mv - filtered_mv) > 100 && !settling) {
    settling = true;
    settling_since = now;
    edge_baseline = raw_mv;
  }
  if (settling) {
    displayed_percent = -1;
    if (abs(raw_mv - edge_baseline) > 40) {
      settling_since = now;
      edge_baseline = raw_mv;
    }
    if (now - settling_since >= 3000) {
      filtered_mv = raw_mv;
      settling = false;
    }
  } else filtered_mv = (filtered_mv * 3 + raw_mv) / 4;
  if (!settling) {
    int target = battery_estimate_percent(filtered_mv);
    if (displayed_percent < 0 || abs(target - displayed_percent) >= 2 || target == 0)
      displayed_percent = target;
  }
  latest_reading.filtered_mv = filtered_mv;
  latest_reading.percent = displayed_percent;
  latest_reading.valid = true;
  update_widget();
  Serial.printf("Battery: ADC=%dmV cell=%dmV filtered=%dmV estimate=%d%%%s; charge state unknown\n",
                adc, raw_mv, filtered_mv, displayed_percent,
                settling ? " (settling)" : "");
}

void battery_status_init(void) {
  if (sample_timer) return;
  pinMode(LCD_RESET_BATTERY_PIN, INPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(LCD_RESET_BATTERY_PIN, ADC_11db);
  // No stored SOC: a saved USB reading must never masquerade as a new measurement.
  sample_timer = lv_timer_create(sample_battery, 50, nullptr);
}

static void widget_deleted(lv_event_t *event) {
  if (widget == lv_event_get_target(event)) {
    widget = battery_icon = battery_label = nullptr;
  }
}

void battery_status_attach(lv_obj_t *parent) {
  widget = ui_block(parent, 0, 0, 76, 20, UI_BG);
  lv_obj_set_style_bg_opa(widget, LV_OPA_TRANSP, 0);
  lv_obj_align(widget, LV_ALIGN_TOP_RIGHT, -4, 2);
  battery_icon = ui_label(widget, LV_SYMBOL_BATTERY_EMPTY);
  lv_obj_align(battery_icon, LV_ALIGN_LEFT_MID, 0, 0);
  battery_label = ui_label(widget, "--%");
  lv_obj_align(battery_label, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_obj_add_event_cb(widget, widget_deleted, LV_EVENT_DELETE, nullptr);
  update_widget();
}

int battery_status_percent(void) { return displayed_percent; }
int battery_status_millivolts(void) { return filtered_mv; }

BatteryReading battery_status_reading(void) { return latest_reading; }
