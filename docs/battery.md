# Battery and diagnostics

[← Project](../README.md) · [Documentation](README.md)

The camera uses a **KBT 883545PL, 3.7 V / 1500 mAh LiPo**. Its readout is a
live voltage-based estimate, marked `~`. It is not a calibrated measurement
of remaining capacity. `--%` means the reading is unavailable or settling.

The [October 17 test](battery-test-2026-10-17/report.md) recorded stable readings
near **3.76 V on battery**, compared with **4.17 V on the first USB baseline**.
The power-dependent jump needs electrical measurements to isolate its cause;
it does not show that the battery instantly lost that much charge.

## How the readout works

- Samples calibrated ADC voltage every 50 ms without blocking the UI.
- Uses a trimmed 21-sample window to reject spikes.
- Shows `--%` after three invalid/noisy windows. Steps over 100 mV trigger
  settling until three seconds of stable readings.
- Uses live readings, with no saved percentage or timed charge increases.
- Logs ADC, derived cell and filtered voltages at 115200 baud.

The board has no dedicated software-readable VBUS, charger-status or current
input in this firmware. Charging is handled by hardware. The blue LED is the
board's indicator: Freenove documents blinking during charging **or with no
battery**, and steady when full. The firmware does not infer charging from
small voltage changes.

## Circuit and limitations

The [FNK0102A schematic](https://github.com/Freenove/Freenove_Media_Kit_for_ESP32-S3/blob/main/Schematic_diagram/SCH_for_Media_Board_1.14inch.pdf)
connects GPIO20 through R16 (10 kΩ) to BAT-B, R12 (10 kΩ) to 3V3, and R17
(20 kΩ) to ground. GPIO20 also resets the LCD through R6, so it returns to
high impedance after the reset pulse.

[Freenove's tutorial](https://docs.freenove.com/projects/fnk0102/en/latest/fnk0102/codes/Main/2_Battery_Voltage_Detection.html)
gives the divider equation:

```text
Vcell_mV = 2.5 × Vadc_mV − 3300
```

This assumes a 3.300 V reference rail. ADC calibration does not remove rail
error, resistor tolerance or load effects; the divider multiplies ADC error
by 2.5. See [Espressif's ADC API](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html)
and [ADC limitations](https://github.com/espressif/esp-faq/blob/master/docs/en/software-framework/peripherals/adc.rst).

The connector is on BAT-A, with Q2 between BAT-A and the sensed BAT-B node.
A switched/disconnected battery and a USB-driven charger-side node can
therefore differ from actual cell voltage. This is a possible explanation,
not an isolated cause established by the test.

The percentage curve is generic; no measured discharge curve for this KBT
cell is included. [TI's SOC estimation note](https://www.ti.com/lit/an/sluaaa1/sluaaa1.pdf)
explains the need to characterize voltage under the application's load.

## Serial commands

Connect **USB UART** at **115200 baud**. Native USB pins GPIO19/20 are already
used by the controls and battery circuitry.

| Command | Action |
| --- | --- |
| `storage` | Exact SD used/free/total bytes, photo count/size, onboard flash and firmware size |
| `battery status` | Show whether test recording is armed |
| `battery start` | Reset `/battery-test.csv` and arm recording across power cycles |
| `battery mark <note>` | Add a phase note while recording |
| `battery stop` | Disarm recording and keep the CSV |
| `battery dump` | Print the CSV between `BATTERY_LOG_BEGIN` / `BATTERY_LOG_END` |

### Recording an unplugged test

1. Send `storage`, then `battery start` and `battery mark USB baseline`.
   Leave Home open for 30 seconds.
2. Send `battery mark unplug next`. Unplug USB, turn on battery power if
   needed, and leave Home open for one minute.
3. Open SHOOT and leave the viewfinder running for one minute.
4. Reconnect USB UART, send `battery mark USB reconnected`, and wait 30 seconds.
5. Send `battery stop`, then `battery dump` to export the results.

The CSV records voltage, estimate, validity, screen, boot ID and uptime every
five seconds. Each record closes the file. Recording resumes after reboot via
`/battery-test.enabled` and stops at 20 minutes per boot or 32 KiB.
Starting a new test replaces only the diagnostic log; photos stay in `/picture/`.

SD usage includes logs and allocated FAT clusters. K/M/G use decimal units,
so filesystem usage can exceed the sum of photo file lengths.

## Calibration

1. Compare serial `ADC=`, `cell=` and `filtered=` with multimeter readings at
   the battery connector and 3V3/GND, both on USB and under battery-only load.
   Use the SD recorder for the unplugged phase; reconnecting USB changes it.
2. Set `BATTERY_REFERENCE_MV` in [`battery_math.h`](../mainCamera/battery_math.h)
   to the measured 3V3 rail. Only adjust gain/offset after measuring two points.
3. For uncorrected voltages `x1,x2` and multimeter values `y1,y2`, use:

   ```text
   BATTERY_GAIN_PPM = round(1000000 × (y2 − y1) / (x2 − x1))
   BATTERY_OFFSET_MV = round(y1 − x1 × BATTERY_GAIN_PPM / 1000000)
   ```

4. Rebuild and verify both points. Characterize discharge under camera load
   before changing the percentage curve. Do not fit it to old screen percentages.

A current-aware fuel gauge would provide a better basis for remaining charge
across charging and discharge. Current, capacity and multimeter calibration
remain unmeasured in the published test.
