# Battery and storage test — October 17, 2026

[← Battery guide](../battery.md) · [Documentation](../README.md)

Diagnostic firmware was installed and verified on the ESP32-S3 camera via
COM12. A bounded CSV recorder on the SD card preserved measurements across
USB removal, battery power-on, viewfinder use and USB reconnection. There are
73 recorded samples plus test markers. Recording has been stopped; the CSV is
retained on SD. Download [raw readings](readings.csv) or [phase statistics](summary.json).

## Measured behavior

These are **ADC-derived readings of the board's BAT-B node**, using the
Freenove divider equation and an assumed 3.300 V reference rail. They have not
been checked against a multimeter at the battery connector. Percentages are
the firmware's generic voltage estimate, not measured remaining charge.

| Phase | Samples | Span between first/last sample | Median reported voltage | Range | Display estimate |
| --- | ---: | ---: | ---: | --- | --- |
| USB baseline | 23 | 110.9 s | 4.1650 V | 4.160-4.172 V | 96% |
| Battery / Home | 35 | 170.8 s | 3.7570 V | 3.755-3.762 V | 40-42% |
| Battery / viewfinder | 8 | 35.3 s | 3.7535 V | 3.750-3.757 V | 40% |
| USB reconnected | 5 | 20.0 s | 3.9870 V | 3.985-4.002 V | 77-79% |

The battery/Home phase stayed within 7 mV over about 171 seconds, from 3.760 V
to 3.755 V. The viewfinder's steady samples stayed within 7 mV, with a median
only 3.5 mV below Home. This short observation shows no large additional
voltage decline from opening the viewfinder. It does not measure runtime or
actual battery capacity.

The approximately 408 mV difference between the first USB baseline and the
battery/Home median is tied to the power condition. The percentage jump is
therefore an unreliable voltage-to-percentage change, not evidence that this
much stored charge was consumed instantly. USB reconnection raised the
reported voltage again, to about 3.99 V and 79%.

### Analysis boundaries

The middle boot also contains samples at 220119 ms (3.822 V, viewfinder) and
225583 ms (3.997 V, Home) during USB reconnection. These are preserved in the
raw CSV but excluded from steady battery medians. The viewfinder's steady
sample span was about 35 seconds; it was not a full discharge/capacity test.
No ADC windows in this export were marked invalid.

## Charging and sensing

The user observed the blue LED blinking with the battery power button ON.
[Freenove's guide](https://docs.freenove.com/projects/fnk0102/en/latest/fnk0102/codes/Main/2_Battery_Voltage_Detection.html)
identifies blinking blue during charging; it also blinks with no battery.
The LED indicates the charger's operating state, without proving the current
actually reaching the cell. Battery current was not measured during this test.

The [official media-board schematic](https://github.com/Freenove/Freenove_Media_Kit_for_ESP32-S3/blob/main/Schematic_diagram/SCH_for_Media_Board_1.14inch.pdf)
places the battery connector on BAT-A, Q2 between BAT-A and BAT-B, and the ADC
divider and charger on BAT-B. Consequently a disconnected/switched battery
and a USB-driven charger-side node can differ from actual cell voltage.
The initial switch state was not confirmed, and the earlier switch-toggle
capture had no confirmed user action. It is a circuit-supported hypothesis,
not a demonstrated isolated cause of the entire measured difference.

The firmware has no dedicated VBUS, charger-status or current input. Charging
is handled by the board hardware. The firmware does not control charging
current, and it should not manufacture a fixed USB correction or restore a
saved percentage to conceal this difference.

To establish the exact electrical cause, compare a multimeter reading at the
battery connector and the 3V3 rail in both power conditions with logged ADC
values. That distinguishes actual cell/load changes, the switched charger
node and reference/divider error. Accurate remaining capacity across charge
and discharge would require a cell/load calibration or an appropriate fuel
gauge. The reported ~40% on battery remains an approximation.

## Storage verified on hardware

| Item | Value | Location |
| --- | ---: | --- |
| SD usable filesystem | 31,255,953,408 bytes (31.26 GB) | External MicroSD |
| SD used before test | 1,703,936 bytes (1.70 MB) | All allocated SD files/directories |
| Photos | 25 files, 1,621,800 logical bytes (1.62 MB) | SD `/picture/` |
| SD used after test | 1,736,704 bytes (1.74 MB) | Includes retained diagnostic CSV |
| ESP flash capacity | 16,777,216 bytes (16 MiB) | Onboard flash |
| Installed firmware | 740,000 bytes | Onboard application partition |

The photo count and logical bytes were unchanged by the test. The old UI
rounded roughly 0.0017 GB to one decimal place, displaying 0.0 GB. The updated
UI uses K/M/G units: about `1.7M/31.3G` at this usage. Filesystem allocation
includes whole FAT clusters and directories, so used space is larger than the
sum of photo file lengths. The retained CSV accounts for another 32 KiB of
allocated space in this test.

The SD card stores photos and this diagnostic log. Firmware uploading writes
the ESP32's onboard flash. This firmware has no SD-card firmware updater.

## Validation and recovery

The native LVGL checks passed, including small-unit formatting, exact storage
reports and persistent bounded logger start/resume/export/stop behavior.
The uploaded binary's data hash was verified by esptool. Hardware serial
commands confirmed logger resume after battery boot, successful CSV export,
explicit stop, and the same 25 photo files afterward.
