#pragma once
#include <stdint.h>
#include <stdio.h>

// Decimal units match SD-card labels. Never round a nonempty card down to 0.
inline void storage_format(char *out, size_t size, uint64_t used, uint64_t total) {
  char amount[24];
  if (used < 1000000ULL) {
    snprintf(amount, sizeof(amount), "%luK", static_cast<unsigned long>((used + 999) / 1000));
  } else {
    uint64_t unit = used < 1000000000ULL ? 1000000ULL : 1000000000ULL;
    unsigned long tenths = (used * 10 + unit / 2) / unit;
    snprintf(amount, sizeof(amount), "%lu.%lu%s", tenths / 10, tenths % 10,
             unit == 1000000ULL ? "M" : "G");
  }
  unsigned long tenths = (total + 50000000ULL) / 100000000ULL;
  snprintf(out, size, "%s/%lu.%luG", amount, tenths / 10, tenths % 10);
}
