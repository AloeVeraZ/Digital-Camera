<div align="center">

# Aloe Camera · ESP32-S3 Digital Camera

### A custom embedded digital camera developed across two hardware generations with live LVGL viewfinder

[![Platform](https://img.shields.io/badge/Platform-ESP32--S3-e7352f?style=flat-square&logo=espressif&logoColor=white)](https://www.espressif.com/)
[![Firmware](https://img.shields.io/badge/Firmware-Arduino_C%2B%2B-6f42c1?style=flat-square&logo=arduino&logoColor=white)](CameraCode/)
[![UI Framework](https://img.shields.io/badge/GUI-LVGL_8.4_%2B_TFT__eSPI-00aeef?style=flat-square)](https://lvgl.io/)
[![Storage](https://img.shields.io/badge/Storage-MicroSD_SD__MMC-22c55e?style=flat-square)](#how-v2-works)
[![Status](https://img.shields.io/badge/Status-V2_Production-f57c00?style=flat-square)](#overview)

A standalone handheld digital camera featuring real-time ST7789 TFT display streaming, 5-way directional navigation, PSRAM caching, and local SD card image management.

<strong>Quick navigation:</strong><br>
[Project Overview](#overview) | [Bill of Materials](#bill-of-materials) | [Known Limitation](#known-limitation-battery-readout) | [Project Media](#project-media) | [Hardware Evolution](#version-1-vs-version-2) | [How V2 Works](#how-v2-works) | [Arduino Setup](#v2-setup-and-flashing) | [Firmware Source](mainCamera/)

</div>

---

## Overview

**Aloe Camera** is a compact, custom embedded digital camera developed across two distinct hardware generations: an ultra-minimal screenless prototype using the Seeed Studio XIAO ESP32S3 Sense, and a fully integrated handheld camera built on the Freenove ESP32-S3 Media Kit with a live color LCD viewfinder.

| System Parameter | Technical Specification |
| --- | --- |
| Main microcontroller | Espressif ESP32-S3 (Dual-core Xtensa LX7 @ 240 MHz, 8 MB Octal PSRAM) |
| Image sensor | OV2640 2-Megapixel CMOS camera module |
| User interface | 1.14" ST7789 IPS TFT display (135 × 240 pixels) powered by LVGL 8.4 |
| Physical navigation | 5-way tactile directional switch (Up, Down, Left, Right, Center Click) |
| Local storage | Removable MicroSD card via 4-bit high-speed SD_MMC bus |
| Power & battery | Integrated LiPo battery management with experimental ADC voltage telemetry |
| Active firmware | Complete 17-module Arduino C++ sketch in [`CameraCode/`](CameraCode/) |

> [!NOTE]
> **Status:** Version 2 represents the active, stable firmware baseline. The complete compilation and upload instructions are documented below.

### Known Limitation: Battery Readout

> [!WARNING]
> The V2 battery percentage is not reliable. When the camera is unplugged, the
> on-screen readout currently reports approximately **20%**, even when that may
> not reflect the battery's actual state of charge. Multiple calibration,
> filtering, persistence, and USB-bias-handling approaches were tested, but a
> dependable percentage reading was not achieved. Treat the displayed value as
> experimental rather than an accurate remaining-capacity measurement.

### Enclosure CAD

The complete V2 enclosure is included in two formats:

| Format | File | Best For |
| --- | --- | --- |
| **STEP** | [Download `aloe-camera-full-case.step`](CAD/aloe-camera-full-case.step) | Importing into other CAD programs, modifying geometry, or preparing the case for manufacturing |
| **Autodesk Fusion archive (F3Z)** | [Download `aloe-camera-full-case.f3z`](CAD/aloe-camera-full-case.f3z) | Opening the original case design in Autodesk Fusion |

The CAD files cover the printed enclosure only; a printer and filament are not
included in the bill of materials below.

## Bill of Materials

The following project-specific parts are required to build one Aloe Camera V2.
A 3D printer and filament are intentionally excluded.

| Qty. | Part | Purpose | Purchase Link |
| ---: | --- | --- | --- |
| 1 | **Freenove ESP32-S3 Media Kit (FNK0102A)** | Main board, OV2640 camera, display, controls, and MicroSD interface | Search for model `FNK0102A` |
| 1 | **KBT 883545PL 3.7 V 1500 mAh LiPo battery** (2-pin 2.54 mm JST, approximately 45 × 35 × 8.8 mm) | Portable power for the completed camera | [Amazon — B09SV2DF7K](https://www.amazon.com/dp/B09SV2DF7K) |

> [!NOTE]
> The linked KBT 883545PL is the battery selected for this build. Confirm the
> connector polarity before plugging it into the board; matching JST connector
> housings do not always guarantee matching wire polarity.

## Project Media

<p align="center">
  <img src="docs/images/aloe-camera-v1-v2.png" alt="Aloe Camera Version 1 and Version 2 hardware side by side" width="900">
</p>

<p align="center"><em>Aloe Camera V1 (left) and the smaller, screen-equipped V2 (right).</em></p>

| Build | Video Demo | What It Shows |
| --- | --- | --- |
| **Version 1** | [Watch the V1 YouTube Short](https://youtube.com/shorts/e1hyfsS4Pfs?si=TT9GlTVhTSVb_hjN) | The original screenless XIAO ESP32S3 Sense prototype. |
| **Version 2** | [Watch the V2 YouTube Short](https://youtube.com/shorts/tKdeSfX_wso?si=DUUyIVA3gJq0QIom) | The redesigned standalone camera with its integrated display and on-device interface. |

### Version 2 Close-Up

<p align="center">
  <img src="docs/images/aloe-camera-v2-close-up.png" alt="Front close-up of the Aloe Camera Version 2 showing its color display and physical controls" width="900">
</p>

<p align="center"><em>V2 running its on-device camera interface, with the color display and physical navigation controls visible.</em></p>

## Version 1 vs. Version 2

Version 1 established the capture concept with the Seeed Studio XIAO ESP32S3
Sense, but its screenless workflow required blind framing and Wi-Fi image
retrieval. See the original prototype in the
[V1 video demo](https://youtube.com/shorts/e1hyfsS4Pfs?si=TT9GlTVhTSVb_hjN).

Version 2 turns that experiment into a self-contained handheld camera. The
Freenove ESP32-S3 Media Kit adds a live color viewfinder, physical menu
navigation, MicroSD storage, image playback, and battery telemetry. See the
finished interaction in the
[V2 video demo](https://youtube.com/shorts/tKdeSfX_wso?si=DUUyIVA3gJq0QIom).

<table>
  <tr>
    <td align="center"><strong>01 / V1 Prototype</strong><br>Seeed XIAO ESP32S3 Sense</td>
    <td align="center">&rarr;<br>Blind capture and Wi-Fi download</td>
    <td align="center"><strong>02 / Usability Review</strong><br>No live viewfinder</td>
    <td align="center">&rarr;<br>Integrated-screen redesign</td>
    <td align="center"><strong>03 / V2 Handheld</strong><br>Freenove ESP32-S3 Media Kit</td>
    <td align="center">&rarr;<br>Live preview and playback</td>
    <td align="center"><strong>04 / Standalone Camera</strong><br>On-device capture workflow</td>
  </tr>
</table>

| Feature / Subsystem | Version 1 (Prototype) | Version 2 (Production Handheld) |
| --- | --- | --- |
| Core board | Seeed Studio XIAO ESP32S3 Sense | Freenove ESP32-S3 Media Kit (FNK0102A) |
| Viewfinder | None (blind capture) | 1.14" Color IPS LCD (135 × 240) live preview |
| Image retrieval | Local Wi-Fi web server download | Direct MicroSD removal + on-screen playback |
| Image caching | Direct flash write | PSRAM high-speed frame buffer caching |
| User interface | Single physical push button + LED | Full LVGL graphical menus, icons, and status bar |
| Power telemetry | Unmonitored | Filtered ADC battery level and charge state indicator |
| Video demo | [Watch V1](https://youtube.com/shorts/e1hyfsS4Pfs?si=TT9GlTVhTSVb_hjN) | [Watch V2](https://youtube.com/shorts/tKdeSfX_wso?si=DUUyIVA3gJq0QIom) |

## How V2 Works

The firmware in [`CameraCode/`](CameraCode/) keeps the display, camera, controls, battery reading, and SD card work separated into non-blocking tasks:

1. **Hardware Initialization:** Boots the ESP32-S3, allocates frame buffers in 8 MB PSRAM, and mounts the MicroSD card over 4-bit `SD_MMC`.
2. **Graphical Rendering:** LVGL 8.4 and TFT_eSPI render responsive menus, status bars, and UI dialogs to the ST7789 panel.
3. **Capture Pipeline:** Pulls JPEG frames from the OV2640 sensor, renders live preview scaling to the screen, and writes full-resolution captures to `/DCIM/`.
4. **Playback Engine:** Decodes and scales stored images from MicroSD into PSRAM for fluid gallery browsing.
5. **Battery Monitor:** Samples battery voltage through a software RC filter and estimates charge percentage. The unplugged percentage remains unreliable; see the [known limitation](#known-limitation-battery-readout).

## V2 Setup and Flashing

### 01 / Prerequisites
- **Arduino IDE** (v2.0+)
- **ESP32 Arduino Core:** Version `3.0.7` (the firmware is also compile-verified with `3.3.11`)
- **Libraries:** No separate download is required. Known-good copies of LVGL `8.4.0`, TFT_eSPI `2.5.43`, the FNK0102A display setup, and Freenove WS2812 `2.0.0` are committed in [`Freenove-Libraries/`](Freenove-Libraries/).

On Windows, install all bundled libraries into the Arduino sketchbook by
opening PowerShell in this repository and running:

```powershell
powershell -ExecutionPolicy Bypass -File .\install-arduino-libraries.ps1
```

The installer automatically uses the OneDrive Documents location when it is
present, preserves existing library folders in a backup directory, and installs
the exact bundled versions. Restart Arduino IDE afterward. To use a custom
Arduino sketchbook location, pass `-SketchbookPath "C:\path\to\Arduino"`.

For manual installation, use **Sketch > Include Library > Add .ZIP Library**
and add every ZIP in [`Freenove-Libraries/`](Freenove-Libraries/) in this order:
`lvgl.zip`, `TFT_eSPI_Setups.zip`, `TFT_eSPI.zip`, then
`Freenove_WS2812_Lib_for_ESP32.zip`.

The archives came from the official [Freenove ESP32-S3 Media Kit
repository](https://github.com/Freenove/Freenove_Media_Kit_for_ESP32-S3/tree/main/Libraries).
The bundled `TFT_eSPI_Setups.zip` differs only by selecting the
`FNK0102A_1P14_135x240_ST7789` display profile.

| Library archive | Version | SHA-256 |
| --- | --- | --- |
| `lvgl.zip` | 8.4.0 | `d687a7cec1466743e9d1b9c7f4f4b033a9121a17d8d5d0ae6461eeb284e84f20` |
| `TFT_eSPI.zip` | 2.5.43 | `b471fc7af1daecd8b022f8e2ec4ca3da8301564676c9e3b6986e51ff2762787b` |
| `TFT_eSPI_Setups.zip` | FNK0102A profile | `05aae81c0afc69e1ab3ce5b5d4d30972d0c9343296c413f8866c28b612eb3fc4` |
| `Freenove_WS2812_Lib_for_ESP32.zip` | 2.0.0 | `9fe4003730d85ef5e9e8f2ed707b57825b62c36e1e1d53409c07feb2c756e7a2` |

### 02 / Board Configuration Settings

| Parameter | Required Arduino IDE Setting |
| --- | --- |
| Board | **ESP32S3 Dev Module** |
| CPU Frequency | **240 MHz (WiFi)** |
| Flash Mode | **QIO 80 MHz** |
| Flash Size | **16 MB (128 Mb)** |
| Partition Scheme | **16M Flash (2MB APP / 12.5MB FATFS)** |
| PSRAM | **OPI PSRAM** |

### 03 / Display Profile Selection

The bundled setup already selects the 1.14-inch FNK0102A display. If using a
separately downloaded Freenove setup, ensure only this compact-display macro is
uncommented in `TFT_eSPI_Setups/User_Setup_Select.h`:

```cpp
#define FNK0102A_1P14_135x240_ST7789
//#define FNK0102B_3P5_320x480_ST7796
```

### 04 / Upload Procedure
1. Insert a FAT32-formatted MicroSD card into the slot.
2. Open [`CameraCode/mainCamera.ino`](CameraCode/mainCamera.ino) in Arduino IDE.
3. Select your serial COM port, compile, and upload.

## Repository Contents

```text
.
├── CAD/                # STEP export and Autodesk Fusion enclosure archive
├── docs/images/        # Project photography used by this README
├── mainCamera/         # Complete 17-file Aloe Camera V2 Arduino C++ firmware
├── Freenove-Libraries/ # Bundled board and display libraries
├── install-arduino-libraries.ps1
├── .gitattributes
├── .gitignore          # Build artifacts and local exclusions
└── README.md           # Engineering documentation and bring-up guide
```
