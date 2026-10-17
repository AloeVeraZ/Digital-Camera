#pragma once
#include <stdint.h>

// Board voltage divider: R16=10k to BAT-B, R12=10k to 3V3,
// R17=20k to GND. Vcell = 2.5 * Vadc - V3V3.
// Override only after comparison with a multimeter; see docs/battery.md.
#ifndef BATTERY_REFERENCE_MV
#define BATTERY_REFERENCE_MV 3300
#endif
#ifndef BATTERY_GAIN_PPM
#define BATTERY_GAIN_PPM 1000000
#endif
#ifndef BATTERY_OFFSET_MV
#define BATTERY_OFFSET_MV 0
#endif

inline int battery_cell_mv(int adc_mv) {
  int mv = adc_mv * 5 / 2 - BATTERY_REFERENCE_MV;
  return static_cast<int>(static_cast<int64_t>(mv) * BATTERY_GAIN_PPM / 1000000)
         + BATTERY_OFFSET_MV;
}

// Approximate single-cell LiPo voltage curve, NOT a KBT discharge calibration.
// Voltage under camera load and while charging is not resting state of charge.
inline int battery_estimate_percent(int mv) {
  static const int volts[] = {3200, 3400, 3500, 3600, 3700, 3750, 3800,
                             3850, 3900, 4000, 4100, 4200};
  static const int levels[] = {0, 3, 6, 12, 25, 40, 50, 60, 70, 80, 90, 100};
  if (mv <= volts[0]) return 0;
  for (unsigned i = 1; i < sizeof(volts) / sizeof(volts[0]); ++i) {
    if (mv <= volts[i]) return levels[i-1] + (mv - volts[i-1]) *
        (levels[i] - levels[i-1]) / (volts[i] - volts[i-1]);
  }
  return 100;
}
