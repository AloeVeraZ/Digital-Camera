# Battery measurement research

The bill of materials identifies a **KBT 883545PL, single-cell 3.7 V,
1500 mAh LiPo**. Capacity affects runtime; it does not determine voltage-based
charge percentage. No verified discharge curve for this specific KBT cell is
included in the project or board documentation. The firmware curve is an
approximation, not a measured KBT calibration.

## Hardware evidence

The [official FNK0102A media-board schematic](https://github.com/Freenove/Freenove_Media_Kit_for_ESP32-S3/blob/main/Schematic_diagram/SCH_for_Media_Board_1.14inch.pdf)
shows GPIO20 connected through R16 (10 kohm) to BAT-B, R12 (10 kohm) to 3V3,
and R17 (20 kohm) to ground. GPIO20 also drives LCD reset through R6.
It must return to high impedance after the boot reset pulse.

[Freenove's voltage tutorial](https://docs.freenove.com/projects/fnk0102/en/latest/fnk0102/codes/Main/2_Battery_Voltage_Detection.html)
derives `Vcell_mV = 2.5 * Vadc_mV - 3300`. This is a biased divider, not a
plain divide-by-two circuit. The offset assumes a 3.300 V rail. The tutorial
also documents USB powering the board while charging the cell, and the blue
LED blinking while charging, steady when full, and blinking with no battery.

The schematic routes the PMIC LED to board indicators, without an MCU charger
status input. The firmware has no reliable charging-current, VBUS, or fuel-gauge
measurement. Rising voltage is not proof of charging: camera load changes and
ADC noise also move voltage. The software therefore does not invent a charging
animation.

[Espressif's analogReadMilliVolts documentation](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html)
describes a calibrated ADC voltage. This does not remove resistor tolerance,
rail error, or wiring/load effects. The divider amplifies ADC error by 2.5.
[Espressif's ADC FAQ](https://github.com/espressif/esp-faq/blob/master/docs/en/software-framework/peripherals/adc.rst)
also documents limitations near the upper ADC range.

## Why 97% could become 16%

The old code restored percentage from Preferences, inferred charging from a
voltage rise as small as 3 mV, added 1% every three minutes without measuring
charge, then recalculated on a detected unplug edge. Its curve mapped 3.72 V
to 16%. Both a stale/inflated saved estimate and an actual voltage change could
produce the reported jump. **Which contribution dominates on this physical
camera requires measurements; it has not been established.**

Charging voltage differs from voltage under camera load at the same charge.
[TI's SOC estimation application note](https://www.ti.com/lit/an/sluaaa1/sluaaa1.pdf)
describes deriving a voltage/SOC curve under the application's discharge load.
Voltage-only estimation cannot deliver fuel-gauge accuracy in both conditions.

## Firmware changes

- One calibrated sample every 50 ms; a trimmed 21-sample window rejects spikes
  without sleeping in the LVGL loop.
- Live readings only: no stored percentage, invented initial 50%, timed charge
  increases, or inferred charger status.
- Invalid/noisy readings become `--%` after three windows. A step over 100 mV
  temporarily shows `--%` and requires three seconds of settled readings.
- `~` identifies an approximate charge estimate. Serial output at 115200 reports
  ADC, cell and filtered voltages, estimate, and settling. Valid low readings
  reach 0%; they are not discarded.
- Reference, gain and offset are configurable in `mainCamera/battery_math.h`.
  Defaults follow the documented circuit, without invented calibration values.

## Calibrating this camera

1. Upload the firmware and open the board's **USB UART** serial connection at
   115200. GPIO19/20 are already used by buttons/battery, so use the UART port.
2. Record serial `ADC=`, `cell=`, and `filtered=` alongside multimeter readings
   across the battery connector and 3V3/GND, with USB connected.
3. Measure battery-only voltage under normal viewfinder load. For battery-only
   serial logging, connect a UART adapter to GND and the board's TX pin, with
   **no adapter power connection**. A charging USB cable changes the condition.
4. If 3V3 differs, set `BATTERY_REFERENCE_MV` to its measured value. If two
   measured points still show systematic error, let `x1,x2` be voltages from the
   uncorrected divider and `y1,y2` the multimeter readings. Set
   `BATTERY_GAIN_PPM = round(1000000*(y2-y1)/(x2-x1))` and
   `BATTERY_OFFSET_MV = round(y1-x1*BATTERY_GAIN_PPM/1000000)`.
   Do not fit corrections to the old on-screen percentages.
5. Rebuild and compare at both points. A measured discharge/runtime profile is
   needed to calibrate the percentage curve. A current-aware fuel gauge would
   be needed for substantially more accurate charge estimation.

## Live hardware observations

COM12 was confirmed as the camera using Arduino CLI's interactive serial
monitor. The first .NET serial-open attempt failed, and a no-reset esptool
connection received no bootloader response; the interactive UART monitor did
succeed. The existing firmware reported approximately 4197-4201 mV with USB
connected, a displayed 99%, and a transition to "charging" on a 3 mV trend.
That directly demonstrates the old heuristic's sensitivity to tiny changes.
It does not establish the battery's actual charge or its battery-only voltage.

A read-only esptool identification confirmed ESP32-S3 revision v0.2, 16 MB
quad flash, and 8 MB embedded PSRAM, matching the repository's flash/OPI-PSRAM
settings. It also returned the device to its existing firmware. The SD card
mounted as SDHC, with about 29808 MiB of filesystem space and 1 MiB used.

The replacement firmware was uploaded to COM12 on October 17, 2026 using
Arduino CLI, with all written regions verified by esptool. Post-upload UART
logs confirmed successful SDHC mounting and live ADC readings around
2995-2997 mV, corresponding to approximately 4.19 V and an estimated 99% while
USB-powered. No crash/reboot loop appeared during the startup observation.

Before uploading, the existing flash region 0x000000-0x20FFFF (bootloader,
partition table, NVS/boot data, and first 2 MiB application slot) was backed up
to `%LOCALAPPDATA%/Digital-Camera/firmware-backups/`. The upload wrote only
onboard boot/partition/application regions; it did not write SD photo files.

A later battery-only voltage test was completed and exported. See the
[October 17 hardware test report](battery-test-2026-10-17/report.md). It confirmed
stable battery-only readings near 3.76 V and a substantial power-dependent
change in the reported voltage. Multimeter, current and capacity measurements
remain outstanding; percentages are not calibrated state-of-charge results.

## Recording an unplugged test

The firmware now accepts serial commands at 115200 and can record to the SD
card while its USB connection is unplugged. It records measured ADC voltage,
the divider-derived cell voltage, filtered voltage, estimated percent, reading
validity, current screen and a boot ID/uptime. It does not infer USB or charge
status. Notes identify phases according to the physical test procedure.

1. Connect the USB UART port and send `storage`. It reports exact SD usable,
   used and free bytes; number/size of images in `/picture`; and onboard flash
   and firmware size. SD space and onboard flash are separate measurements.
2. Send `battery start` to create `/battery-test.csv` and arm resume using
   `/battery-test.enabled`. This resets the previous diagnostic log only.
3. Send `battery mark USB baseline`, and leave the camera at Home for 30 seconds.
4. Send `battery mark unplug next`, then unplug USB, turn the camera on using
   its battery power button if necessary, and leave Home open for one minute.
5. Open SHOOT and leave the live viewfinder running for one minute. This gives
   a camera-load measurement, separate from idle/Home load.
6. Reconnect USB UART. Send `battery mark USB reconnected`, wait 30 seconds,
   then send `battery stop` followed by `battery dump`. Boot IDs distinguish
   a battery-only boot from reconnect/reboot. The log persists across both.

Logs append every five seconds and close the file after each record. Recording
stops after 20 minutes within one boot or a 32 KiB log limit. It can be stopped
earlier explicitly. Test files live outside `/picture` and are not photo files.
The SD usage indicator includes these diagnostic files as part of used space.
K/M/G are decimal KB/MB/GB, so roughly 1 MiB used is displayed as about 1.0M
rather than 0.0 GB. Photo logical bytes can differ from filesystem used bytes
because FAT allocates whole clusters and other files also consume space.

User observation after the first UI firmware: approximately 75% with USB
connected and 39% after unplugging/powering on. The current generic curve maps
those estimates to roughly 3.95 V and 3.75 V; those are inferred ranges, not
new physical measurements. A subsequent direct USB UART observation read about
3.99 V and 79%. Battery-only raw measurements are now in the [completed test report](battery-test-2026-10-17/report.md).

## Completed hardware test

The diagnostic build was flashed and hash-verified. The SD log survived battery
power-on and was exported after reconnection; logging was explicitly stopped.
[Results, phase statistics and raw CSV](battery-test-2026-10-17/report.md) document
about 4.17 V on the first USB baseline, 3.76 V at battery/Home, 3.75 V with the
viewfinder running, and 3.99 V after USB reconnection. Photos remained at
25 files / 1,621,800 logical bytes. SD usage was about 1.70 MB before logging;
the prior zero-GB display was a rounding issue and is fixed with K/M/G units.
