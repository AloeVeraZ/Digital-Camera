<div align="center">

# aloeZ mini mini

[![Platform](https://img.shields.io/badge/Platform-ESP32--S3-e7352f?style=flat-square&logo=espressif&logoColor=white)](https://www.espressif.com/)
[![Firmware](https://img.shields.io/badge/Firmware-Arduino_C%2B%2B-6f42c1?style=flat-square&logo=arduino&logoColor=white)](mainCamera/)
[![UI Framework](https://img.shields.io/badge/GUI-LVGL_8.4_%2B_TFT__eSPI-00aeef?style=flat-square)](https://lvgl.io/)
[![Storage](https://img.shields.io/badge/Storage-MicroSD_SD__MMC-22c55e?style=flat-square)](#how-v2-works)

<img src="docs/images/aloez-mini-mini-v2.png" alt="aloeZ mini mini camera" width="900">

<strong>Quick navigation:</strong><br>
[Overview](#overview) | [Parts](#bill-of-materials) | [CAD](#cad-files) | [Photos and Videos](#photos-and-videos) | [V1 and V2](#v1-and-v2) | [Setup](#v2-setup-and-flashing) | [Code](mainCamera/)

</div>

---

## Overview

aloeZ mini mini is a small digital camera I made with an ESP32-S3. I built V1 first
as a screenless camera, then rebuilt it as V2 with a screen, controls, a battery,
and MicroSD storage. V2 is the final version and the one used in this repo.

The board specs, pinouts, and examples are in the [Freenove documentation](https://docs.freenove.com/projects/fnk0102/en/latest/index.html).
Freenove also has the original [code and board files on GitHub](https://github.com/Freenove/Freenove_Media_Kit_for_ESP32-S3).

## Bill of Materials

These are the parts I bought for V2. This does not include a 3D printer or
filament.

| Qty. | Part | Link |
| ---: | --- | --- |
| 1 | Freenove ESP32-S3 Media Kit, FNK0102A with the 1.14 inch screen | [Amazon](https://www.amazon.com/dp/B0F9L7SBWX) |
| 1 | KBT 883545PL 3.7 V 1500 mAh LiPo battery, about 45 × 35 × 8.8 mm | [Amazon](https://www.amazon.com/dp/B09SV2DF7K) |

> [!NOTE]
> Amazon links do not always stay up. If a link stops working, search for the
> model and specs in the table.

> [!CAUTION]
> The battery I bought had the red and black wires in the wrong positions for
> the Media Kit. I removed the contacts from the plug and swapped them. Check
> the polarity before plugging it in. Red goes to positive and black goes to
> negative or GND. The battery port takes 3.7 to 4.2 V. It is not the 5 V or
> 3.3 V header. Freenove has a [battery wiring diagram here](https://docs.freenove.com/projects/fnk0102/en/latest/fnk0102/codes/Main/Preface.html#battery-optional).

## CAD Files

I included the full case in two formats.

| Format | File |
| --- | --- |
| STEP | [`aloez-mini-mini-full-case.step`](CAD/aloez-mini-mini-full-case.step) |
| Fusion 360 archive | [`aloez-mini-mini-full-case.f3z`](CAD/aloez-mini-mini-full-case.f3z) |

Use the STEP file in most CAD programs. Use the F3Z file if you want the Fusion
360 version.

## Photos and Videos

<p align="center">
  <img src="docs/images/aloez-mini-mini-v1-v2.png" alt="aloeZ mini mini V1 and V2 side by side" width="900">
</p>

<p align="center"><em>V1 is on the left. V2 is on the right.</em></p>

| Version | Video |
| --- | --- |
| V1 | [YouTube Short](https://youtube.com/shorts/e1hyfsS4Pfs?si=TT9GlTVhTSVb_hjN) |
| V2 | [YouTube Short](https://youtube.com/shorts/tKdeSfX_wso?si=DUUyIVA3gJq0QIom) |

## Known Limitation: Battery Readout

> [!WARNING]
> The battery percentage does not work correctly. When the camera is unplugged,
> it usually shows around 20 percent even when that is not the real charge. I
> tried recalibrating it and changing the filtering, but I could not get a
> reliable reading. Do not use the number as an accurate battery measurement.

## V1 and V2

### V1

V1 used a Seeed Studio XIAO ESP32S3 Sense. It had no screen, so I could not see
the photo before taking it. Photos were downloaded through a Wi-Fi page.
[Watch V1 here.](https://youtube.com/shorts/e1hyfsS4Pfs?si=TT9GlTVhTSVb_hjN)

### V2

V2 uses the Freenove ESP32-S3 Media Kit. I added a live viewfinder, menus,
physical controls, a gallery, MicroSD storage, and a battery. This is the final
version of the camera. [Watch V2 here.](https://youtube.com/shorts/tKdeSfX_wso?si=DUUyIVA3gJq0QIom)

## How V2 Works

The code is in [`mainCamera/`](mainCamera/).

- LVGL and TFT_eSPI run the menus and screen.
- The camera feed is scaled down for the live viewfinder.
- Full photos are saved to `/DCIM/` on the MicroSD card.
- Saved photos can be opened from the gallery.
- The battery icon reads the board's voltage, but the percentage is not accurate.

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
2. Open [`mainCamera/mainCamera.ino`](mainCamera/mainCamera.ino) in Arduino IDE.
3. Select your serial COM port, compile, and upload.

## Repository Contents

```text
.
├── CAD/                # STEP export and Autodesk Fusion enclosure archive
├── docs/images/        # Camera photos
├── mainCamera/         # Arduino code for V2
├── Freenove-Libraries/ # Libraries used by the code
├── install-arduino-libraries.ps1
├── .gitattributes
├── .gitignore          # Build artifacts and local exclusions
└── README.md           # Project information and setup
```
