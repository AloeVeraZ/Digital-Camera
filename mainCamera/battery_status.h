#ifndef __BATTERY_STATUS_H
#define __BATTERY_STATUS_H

#include "lvgl.h"

struct BatteryReading {
  uint32_t sampled_ms;
  uint32_t sequence;
  int adc_mv;
  int cell_mv;
  int filtered_mv;
  int percent;
  bool valid;
};
BatteryReading battery_status_reading(void);
void battery_status_init(void);
void battery_status_attach(lv_obj_t *parent);
int battery_status_percent(void);
int battery_status_millivolts(void);

#endif
