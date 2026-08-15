<div align="center">

# Aloe Camera

### A custom ESP32-S3 digital camera developed across two hardware versions

[Overview](#overview) · [Version 1](#version-1) · [Version 2](#version-2) · [V2 setup](#v2-setup) · [Repository contents](#repository-contents)

</div>

---

## Overview

**Aloe Camera** is a compact digital camera project designed and developed by [Angelo James Demetroulakos](https://angelojamesny.com/). The project went through two versions: an extremely simple screenless camera built around a Seeed Studio XIAO ESP32S3 Sense, followed by a more complete handheld camera built with the Freenove Media Kit for ESP32-S3.

The first version proved that a small ESP32 camera could capture and store photographs. The second version focused on making the camera practical to use by adding a live viewfinder, on-device image playback, removable storage, a custom interface, and visible battery information.

> **Status:** V2 is the current version. Its Arduino source is included in [`mainCamera`](mainCamera/). The original V1 source is no longer available because that prototype and its firmware are outdated.

## Version 1

V1 used the [Seeed Studio XIAO ESP32S3 Sense](https://www.amazon.com/dp/B0C69FFVHH), which combines an ESP32-S3 microcontroller with an OV2640 camera module.

The camera was intentionally minimal:

- a power switch turned the camera on;
- an LED indicated that the camera was ready;
- one button captured a photograph;
- there was no screen or live preview; and
- the camera created its own Wi-Fi network for accessing stored images from a phone.

### Why V1 was replaced

V1 worked, but the experience was inconvenient. There was no way to see or compose a photograph before taking it. A phone was required to retrieve the files, and every transfer involved joining the Wi-Fi network created by the camera, browsing its storage, and downloading the images manually.

The prototype demonstrated the basic idea, but it depended too heavily on a second device to function as a usable camera.

### V1 images

| Placeholder 1 | Placeholder 2 |
|:---:|:---:|
| **V1 image placeholder** | **V1 image placeholder** |

## Version 2

V2 uses the [Freenove Media Kit for ESP32-S3](https://www.amazon.com/dp/B0F9L7SBWX), specifically the compact **FNK0102A version with the 1.14-inch 135 × 240 display**.

The Freenove kit provides the main hardware platform in one package:

- ESP32-S3 processor;
- camera module;
- 1.14-inch color display;
- five-way navigation control;
- microSD card slot;
- battery connection and voltage sensing;
- WS2812 status LED; and
- onboard speaker and microphone hardware.

Freenove's example code and configuration files were useful for bringing up the hardware, but the finished camera software was rebuilt and customized for this project.

### V2 improvements

- **Live viewfinder:** The display shows what the camera is pointing at before a photograph is taken.
- **On-device playback:** Saved photographs can be viewed directly on the camera instead of through a phone.
- **Direct storage:** Images are written to the microSD card and can be removed with the card.
- **Custom interface:** V2 uses an original menu layout, icons, camera screen, and photo viewer.
- **Faster operation:** The capture and navigation paths were adjusted to make the interface more responsive.
- **Higher image quality:** Camera settings were changed from the original examples to improve the saved photographs.
- **Faster image loading:** The photo viewer uses ESP32-S3 PSRAM to cache image data before it is displayed.
- **Battery interface:** A custom battery indicator shows percentage and charging state across the main screens.
- **Controlled startup audio:** The firmware controls the speaker pins during startup so the hardware does not produce an uncontrolled power-on ring. The onboard speaker is available for deliberate startup feedback.

### V2 images

| Placeholder 1 | Placeholder 2 |
|:---:|:---:|
| **V2 image placeholder** | **V2 image placeholder** |

| Placeholder 3 | Placeholder 4 |
|:---:|:---:|
| **V2 image placeholder** | **V2 image placeholder** |

## How V2 works

The included Arduino sketch coordinates the camera, display, controls, storage, status LED, and battery interface.

1. The ESP32-S3 initializes the camera and the SD_MMC connection.
2. LVGL and TFT_eSPI draw the custom interface on the 1.14-inch screen.
3. The five-way switch moves between the menu, camera, and photo viewer.
4. Captured photographs are written to the camera folder on the microSD card.
5. The photo viewer loads image data into PSRAM for faster on-device playback.
6. A persistent, filtered battery estimate is shown throughout the interface.

## V2 setup

The complete Freenove download contains the hardware-specific libraries and configuration files required by the sketch. Use the library ZIP files from its `Libraries` folder rather than replacing them with newer Library Manager versions.

### Required software and libraries

- Arduino IDE
- ESP32 Arduino core **3.0.7**
- LVGL **8.4.0** from `lvgl.zip`
- TFT_eSPI **2.5.43** from `TFT_eSPI.zip`
- Freenove display profiles from `TFT_eSPI_Setups.zip`
- Freenove WS2812 library **2.0.0**

The camera, Preferences, filesystem, SD_MMC, and PSRAM APIs are provided by the ESP32 Arduino core. The `ESP32-audioI2S` library included by Freenove is not required by the current V2 sketch.

### Arduino board settings

| Setting | Value |
| --- | --- |
| Board | ESP32S3 Dev Module |
| CPU frequency | 240 MHz (WiFi) |
| Flash mode | QIO 80 MHz |
| Flash size | 16 MB (128 Mb) |
| Partition scheme | 16M Flash (2MB APP / 12.5MB FATFS) |
| PSRAM | OPI PSRAM |

### Select the 1.14-inch display

The Freenove bundle currently enables its 3.5-inch display profile by default. In `TFT_eSPI_Setups/User_Setup_Select.h`, enable the small-screen profile and disable the 3.5-inch profile:

```cpp
#define FNK0102A_1P14_135x240_ST7789
//#define FNK0102B_3P5_320x480_ST7796
```

Only one display profile should be enabled. The same `FNK0102A_1P14_135x240_ST7789` macro selects the small-screen paths inside the V2 source.

### Upload

1. Install the ESP32 core and Freenove library ZIP files.
2. Correct the display profile as shown above.
3. Insert a microSD card into the Freenove Media Kit.
4. Open [`mainCamera/mainCamera.ino`](mainCamera/mainCamera.ino) in Arduino IDE.
5. Apply the board settings, select the serial port, verify, and upload.

## Repository contents

```text
.
├── mainCamera/         # Complete 17-file Aloe Camera V2 Arduino sketch
├── .gitattributes
├── .gitignore          # Local file exclusions
└── README.md           # Project history, hardware, setup, and placeholders
```

CAD files and final project photographs can be added when they are ready for release.

## Author

Aloe Camera was designed and developed by **Angelo James Demetroulakos**.
