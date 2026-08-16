#include "battery_status.h"
#include "Arduino.h"
#include "display.h"
#include <Preferences.h>

static const uint8_t BATTERY_PIN = 20;
static const int BATTERY_EMPTY_MV = 3200;
static const int BATTERY_FULL_MV = 4200;
// Accept real low/high readings so an exhausted cell becomes 0% instead of
// being rejected and leaving an old saved percentage frozen on screen.
static const int BATTERY_SAMPLE_MIN_MV = 2400;
static const int BATTERY_SAMPLE_MAX_MV = 4300;
static const uint32_t SAMPLE_PERIOD_MS = 1000;
static const uint32_t FLOW_PERIOD_MS = 70;
static const uint8_t CHARGE_SEGMENT_COUNT = 10;
static const int CHARGE_RISE_MV = 3;
static const int CHARGE_STRONG_RISE_MV = 8;
static const int DISCHARGE_DROP_MV = 4;
static const int DISCHARGE_FROM_PEAK_MV = 5;
static const uint32_t PERCENT_UPDATE_MS = 5000;
// While USB is present, GPIO20 is biased by the board power/charge circuit.
// Never convert that USB-biased voltage directly into state-of-charge.
static const uint32_t CHARGING_PERCENT_STEP_MS = 180000UL;  // +1% per 3 minutes max
static const uint32_t UNPLUG_SETTLE_MS = 5000UL;

static lv_timer_t *sample_timer = NULL;
static lv_timer_t *flow_timer = NULL;
static lv_obj_t *widget = NULL;
static lv_obj_t *battery_fill = NULL;
static lv_obj_t *battery_label = NULL;
static lv_obj_t *charge_icon = NULL;
static lv_obj_t *charge_segments[CHARGE_SEGMENT_COUNT] = {};
static int filtered_mv = 0;
static int displayed_percent = -1;
static int previous_raw_mv = 0;
static int trend_floor_mv = 0;
static int trend_peak_mv = 0;
static uint8_t positive_windows = 0;
static bool charging = false;
static uint8_t flow_phase = 0;
static uint32_t last_percent_change = 0;
static uint32_t last_charge_percent_step = 0;
static uint32_t unplug_settle_until = 0;
static bool unplug_recalibrate_pending = false;
static uint32_t last_saved = 0;
static bool have_valid_reading = false;
static bool have_live_reading = false;
static Preferences battery_preferences;

// Freenove FNK0102 reference:
// https://docs.freenove.com/projects/fnk0102/en/latest/fnk0102/codes/Main/2_Battery_Voltage_Detection.html
// GPIO20 measures the battery through a biased divider. It is not a VBUS or
// charger-status input, so charging is inferred only from a measured rise.
static int read_battery_millivolts(void) {
  int samples[21];
  int count = 0;
  pinMode(BATTERY_PIN, INPUT);
  for (int i = 0; i < 21; ++i) {
    int adc_mv = analogReadMilliVolts(BATTERY_PIN);
    int value = adc_mv * 5 / 2 - 3300;
    if (value >= BATTERY_SAMPLE_MIN_MV && value <= BATTERY_SAMPLE_MAX_MV)
      samples[count++] = value;
    delay(2);
  }
  // GPIO20 is shared with LCD reset and must remain high impedance after the
  // one-time reset pulse or the battery measurement is biased.
  pinMode(LCD_RESET_BATTERY_PIN, INPUT);
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
  // Average the middle 60% rather than trusting one median reading. This
  // rejects ADC spikes while retaining sub-percent voltage movement.
  int trim = count >= 10 ? count / 5 : 0;
  int total = 0;
  for (int i = trim; i < count - trim; ++i) total += samples[i];
  return total / (count - trim * 2);
}

// Dense resting-voltage curve for a 1-cell 3.7 V LiPo. Capacity (2000 mAh)
// changes runtime, not these voltage endpoints. The dense middle section
// avoids the old curve reporting about 64% around the nominal-voltage plateau.
static int voltage_to_percent(int mv) {
  static const int volts[] = {
    3200, 3400, 3500, 3600, 3680, 3700, 3720, 3740, 3760,
    3780, 3800, 3820, 3840, 3860, 3880, 3900, 3930, 3960,
    4000, 4050, 4100, 4150, 4200
  };
  static const int percents[] = {
     0,  2,  4,  6, 10, 13, 16, 20, 25,
    30, 35, 40, 48, 55, 60, 65, 70, 75,
    80, 85, 90, 95, 100
  };
  static const int point_count = sizeof(volts) / sizeof(volts[0]);
  if (mv <= BATTERY_EMPTY_MV) return 0;
  if (mv >= BATTERY_FULL_MV) return 100;
  for (int i = 1; i < point_count; ++i) {
    if (mv <= volts[i]) {
      int percent = percents[i - 1] + (mv - volts[i - 1]) *
                    (percents[i] - percents[i - 1]) / (volts[i] - volts[i - 1]);
      return max(1, percent);
    }
  }
  return 100;
}

static lv_color_t battery_color(int percent) {
  if (percent < 10) return lv_color_hex(0xFF453A);
  if (percent < 30) {
    // Move from orange at 10% toward warm yellow just before 30%.
    int amount = (percent - 10) * 255 / 19;
    int red = 255;
    int green = 149 + (214 - 149) * amount / 255;
    int blue = 10 * amount / 255;
    return lv_color_make(red, green, blue);
  }
  return lv_color_hex(0x34C759);
}

static lv_color_t charging_segment_color(int index) {
  if (displayed_percent < 10) {
    return lv_color_hex(index & 1 ? 0xFF5A52 : 0xFF312A);
  }
  if (displayed_percent < 30) {
    return lv_color_hex(index & 1 ? 0xFFD60A : 0xFF9500);
  }
  return lv_color_hex(index & 1 ? 0x30D158 : 0x248A3D);
}

static void draw_charge_segments(void) {
  if (!battery_fill || !lv_obj_is_valid(battery_fill)) return;

  int current_segments = displayed_percent <= 0 ? 0 :
                         (displayed_percent + 9) / 10;
  current_segments = min(current_segments, (int)CHARGE_SEGMENT_COUNT);

  int progress = min((int)flow_phase, 100);
  int remaining = CHARGE_SEGMENT_COUNT - current_segments;
  int animated_segments = remaining * progress / 100;
  int visible_segments = current_segments + animated_segments;

  for (int i = 0; i < CHARGE_SEGMENT_COUNT; ++i) {
    lv_obj_t *segment = charge_segments[i];
    if (!segment || !lv_obj_is_valid(segment)) continue;
    lv_obj_set_style_bg_color(segment, charging_segment_color(i), 0);
    if (charging && i < visible_segments) {
      lv_obj_clear_flag(segment, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(segment, LV_OBJ_FLAG_HIDDEN);
    }
  }
}

static void update_widget(void) {
  if (!widget || !lv_obj_is_valid(widget)) return;
  if (displayed_percent < 0) {
    lv_label_set_text(battery_label, "--");
    lv_obj_set_width(battery_fill, 1);
  } else {
    lv_label_set_text_fmt(battery_label, "%d", displayed_percent);
  }

  if (charging) {
    lv_obj_set_width(battery_fill, 32);
    lv_obj_set_style_bg_opa(battery_fill, LV_OPA_TRANSP, 0);
    draw_charge_segments();
    lv_obj_clear_flag(charge_icon, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_set_width(battery_fill,
        displayed_percent < 0 ? 1 : 1 + displayed_percent * 31 / 100);
    lv_obj_set_style_bg_color(battery_fill, battery_color(displayed_percent), 0);
    lv_obj_set_style_bg_opa(battery_fill, LV_OPA_COVER, 0);
    draw_charge_segments();
    lv_obj_add_flag(charge_icon, LV_OBJ_FLAG_HIDDEN);
  }
}

static void animate_charge_flow(lv_timer_t *) {
  if (!charging || !battery_fill || !lv_obj_is_valid(battery_fill)) return;

  // Keep the real charge portion filled, then add only the missing 10%
  // segments until the animation reaches 100%. Hold briefly before repeating.
  flow_phase = (flow_phase + 2) % 120;
  draw_charge_segments();
}

static void reset_voltage_trend(int raw_mv) {
  trend_floor_mv = raw_mv;
  trend_peak_mv = raw_mv;
  positive_windows = 0;
}

static void update_usb_power_state(int raw_mv) {
  bool was_charging = charging;

  if (!raw_mv) {
    // An absent/out-of-range battery is not proof of charging. Older code
    // treated this as USB present, which could show charging as a low battery
    // fell below the valid 3.2 V range.
    charging = false;
    previous_raw_mv = 0;
    reset_voltage_trend(0);
    if (was_charging) {
      flow_phase = 0;
      Serial.println("Battery charging: no (no valid rising voltage)");
    }
    return;
  }

  if (!previous_raw_mv) {
    previous_raw_mv = raw_mv;
    reset_voltage_trend(raw_mv);
    return;
  }

  int delta = raw_mv - previous_raw_mv;
  previous_raw_mv = raw_mv;

  // A large downward edge is the most reliable indication that USB power was
  // just removed. Remember it even if the charging heuristic had not yet
  // switched on (for example, immediately after a USB-powered reboot).
  if (delta <= -20) {
    unplug_recalibrate_pending = true;
    unplug_settle_until = millis() + UNPLUG_SETTLE_MS;
  }

  if (!trend_floor_mv || raw_mv < trend_floor_mv) trend_floor_mv = raw_mv;
  if (!trend_peak_mv || raw_mv > trend_peak_mv) trend_peak_mv = raw_mv;

  // A 2000 mAh cell rises slowly on this board's low-current charger. Retain
  // the lowest observed voltage instead of throwing the trend away after a
  // few flat samples. Charging may only turn on on a positive sample, after a
  // 3 mV net rise across several samples or an obvious 8 mV cable-in step.
  if (!charging) {
    if (delta > 0) {
      if (positive_windows < 255) ++positive_windows;
      int net_rise = raw_mv - trend_floor_mv;
      if (delta >= CHARGE_STRONG_RISE_MV ||
          (positive_windows >= 2 && net_rise >= CHARGE_RISE_MV)) {
        charging = true;
        trend_peak_mv = raw_mv;
      }
    } else if (delta <= -2) {
      reset_voltage_trend(raw_mv);
    }
  } else {
    // Unplugging puts the camera load back on the cell and creates a downward
    // edge. Either an immediate drop or a fall from the charging peak stops
    // the animation. Flat voltage keeps the state, which also handles a full
    // 2000 mAh cell whose voltage is no longer visibly increasing.
    if (delta <= -DISCHARGE_DROP_MV ||
        raw_mv <= trend_peak_mv - DISCHARGE_FROM_PEAK_MV) {
      charging = false;
      reset_voltage_trend(raw_mv);
    }
  }

  if (charging != was_charging) {
    flow_phase = 0;
    Serial.printf("Battery charging: %s (step %+dmV, trend %+dmV)\n",
                  charging ? "yes" : "no", delta, raw_mv - trend_floor_mv);
  }
}

static void sample_battery(lv_timer_t *) {
  bool charging_before_sample = charging;
  int raw_mv = read_battery_millivolts();

  update_usb_power_state(raw_mv);
  if (!raw_mv) {
    update_widget();
    Serial.printf("Battery: no valid cell reading, charging=no, retaining %d%%\n",
                  displayed_percent);
    return;
  }

  uint32_t now = millis();

  // The first ADC reading is useful for establishing a voltage baseline, but it
  // must NOT immediately replace a saved battery percentage. If the unit boots
  // while USB is connected, GPIO20 can read much higher and previously caused
  // the exact 29% -> 66% jump seen on screen.
  if (!have_live_reading) {
    filtered_mv = raw_mv;
    have_live_reading = true;
    if (!have_valid_reading || displayed_percent < 0) {
      displayed_percent = voltage_to_percent(raw_mv);
      have_valid_reading = true;
    }
    last_percent_change = now;
    last_charge_percent_step = now;
  }

  // Detect the USB removal edge. Give the cell/load a few seconds to settle,
  // then recalibrate from the true battery-powered voltage.
  if (charging_before_sample && !charging) {
    unplug_recalibrate_pending = true;
    unplug_settle_until = now + UNPLUG_SETTLE_MS;
    last_percent_change = now;
  }

  if (charging) {
    // IMPORTANT: ignore raw/filtered battery voltage for percentage while USB
    // is present. The board's biased divider/power path makes that reading rise
    // immediately even though the battery did not instantly gain capacity.
    // We only allow a very slow +1% progression while charging.
    if (displayed_percent >= 0 && displayed_percent < 100 &&
        now - last_charge_percent_step >= CHARGING_PERCENT_STEP_MS) {
      ++displayed_percent;
      last_charge_percent_step = now;
    }
  } else {
    if (unplug_recalibrate_pending &&
        (int32_t)(now - unplug_settle_until) >= 0) {
      // This is the first trustworthy battery-only reading after USB removal.
      // Snap to it once so any old USB-inflated value (such as 66%) is removed.
      filtered_mv = raw_mv;
      displayed_percent = voltage_to_percent(filtered_mv);
      have_valid_reading = true;
      unplug_recalibrate_pending = false;
      last_percent_change = now;
    } else if (!unplug_recalibrate_pending &&
               (int32_t)(now - unplug_settle_until) >= 0) {
      if (!filtered_mv) filtered_mv = raw_mv;
      else filtered_mv = (filtered_mv * 3 + raw_mv) / 4;

      int target = voltage_to_percent(filtered_mv);
      if (displayed_percent < 0) {
        displayed_percent = target;
      } else if (target != displayed_percent &&
                 now - last_percent_change >= PERCENT_UPDATE_MS) {
        int difference = target - displayed_percent;
        displayed_percent += constrain(difference, -2, 2);
        last_percent_change = now;
      }
    }

    // Persist ONLY battery-powered measurements. Saving while USB is present
    // poisons the next boot with a falsely high percentage.
    if (now - last_saved >= 30000UL || last_saved == 0) {
      battery_preferences.putInt("percent", displayed_percent);
      battery_preferences.putInt("millivolts", filtered_mv);
      last_saved = now;
      have_valid_reading = true;
    }
  }

  update_widget();
  if (charging != charging_before_sample) flow_phase = 0;
  Serial.printf("Battery: raw=%dmV filtered=%dmV shown=%d%% charging=%s%s\n",
                raw_mv, filtered_mv, displayed_percent,
                charging ? "yes" : "no",
                charging ? " (USB voltage ignored for SOC)" : "");
}

void battery_status_init(void) {
  if (sample_timer) return;
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  battery_preferences.begin("battery", false);
  int saved_percent = battery_preferences.getInt("percent", -1);
  have_valid_reading = battery_preferences.isKey("percent") && saved_percent > 1 &&
                       saved_percent <= 100;
  displayed_percent = have_valid_reading ? saved_percent : 50;
  // Keep the last known battery-only percentage across boot. This is critical
  // when booting while USB is already connected, because the first GPIO20
  // reading can be USB-biased and much higher than the true resting voltage.
  int saved_mv = battery_preferences.getInt("millivolts", 0);
  filtered_mv = (saved_mv >= BATTERY_SAMPLE_MIN_MV &&
                 saved_mv <= BATTERY_SAMPLE_MAX_MV) ? saved_mv : 0;
  have_live_reading = false;

  // Charging is deliberately never restored from Preferences. It describes
  // the live power source, not persistent battery data, and restoring it is
  // what made the icon remain green after USB was removed.
  charging = false;
  previous_raw_mv = 0;
  reset_voltage_trend(0);
  battery_preferences.remove("charging");
  sample_battery(NULL);
  sample_timer = lv_timer_create(sample_battery, SAMPLE_PERIOD_MS, NULL);
  flow_timer = lv_timer_create(animate_charge_flow, FLOW_PERIOD_MS, NULL);
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
  lv_obj_clear_flag(battery_fill, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

  for (int i = 0; i < CHARGE_SEGMENT_COUNT; ++i) {
    charge_segments[i] = lv_obj_create(battery_fill);
    lv_obj_remove_style_all(charge_segments[i]);
    lv_obj_set_size(charge_segments[i], 2, 13);
    lv_obj_align(charge_segments[i], LV_ALIGN_LEFT_MID, i * 3, 0);
    lv_obj_set_style_radius(charge_segments[i], 1, 0);
    lv_obj_set_style_bg_opa(charge_segments[i], LV_OPA_COVER, 0);
    lv_obj_clear_flag(charge_segments[i],
                      LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  }

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