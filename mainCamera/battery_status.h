#ifndef __BATTERY_STATUS_H
#define __BATTERY_STATUS_H

#include "lvgl.h"

void battery_status_init(void);
void battery_status_attach(lv_obj_t *parent);
int battery_status_percent(void);
bool battery_status_is_charging(void);

#endif
