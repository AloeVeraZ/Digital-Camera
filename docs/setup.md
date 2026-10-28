# Setup and flashing

[← Project](../README.md) · [Documentation](README.md)

Use the **Freenove FNK0102A Media Kit** with its 1.14-inch, 240 × 135 display.
The firmware was compiled and tested with ESP32 Arduino core **3.3.11**.

## 1. Install the libraries

Install Arduino IDE 2.x and ESP32 Arduino core 3.3.11. The required libraries
are bundled in [`Freenove-Libraries/`](../Freenove-Libraries/): LVGL 8.4.0,
TFT_eSPI 2.5.43, the Freenove display setup, and Freenove WS2812 2.0.0.

On Windows, open PowerShell in the repository and run:

```powershell
powershell -ExecutionPolicy Bypass -File .\install-arduino-libraries.ps1
```

The installer uses OneDrive Documents when present and backs up existing
library folders. Restart Arduino IDE afterward. For a custom sketchbook:

```powershell
.\install-arduino-libraries.ps1 -SketchbookPath 'C:\path\to\Arduino'
```

For manual installation, use **Sketch → Include Library → Add .ZIP Library**
in this order: `lvgl.zip`, `TFT_eSPI_Setups.zip`, `TFT_eSPI.zip`, then
`Freenove_WS2812_Lib_for_ESP32.zip`.

## 2. Configure the board

| Arduino IDE setting | Value |
| --- | --- |
| Board | ESP32S3 Dev Module |
| CPU frequency | 240 MHz (WiFi) |
| Flash mode | QIO 80 MHz |
| Flash size | 16 MB (128 Mb) |
| Partition scheme | 16M Flash (2MB APP / 12.5MB FATFS) |
| PSRAM | OPI PSRAM |

The bundled `TFT_eSPI_Setups/User_Setup_Select.h` already selects:

```cpp
#define FNK0102A_1P14_135x240_ST7789
//#define FNK0102B_3P5_320x480_ST7796
```

Keep only the FNK0102A profile enabled if replacing the bundled setup.

## 3. Upload

1. Open [`mainCamera/mainCamera.ino`](../mainCamera/mainCamera.ino).
2. Connect the board's **USB UART** port using a data cable and select its COM port.
3. Compile and upload. Close any serial monitor using that port first.
4. Insert a FAT32 MicroSD card to take and save photos.

GPIO19/20 are used by the controls and battery circuitry. Use USB UART rather
than configuring those pins for native USB.

Firmware goes into the board's **16 MiB onboard SPI flash**. The SD card is
separate: photos live in `/picture/`, with optional battery logs at the root.
Uploading does not require an SD card. There is no SD firmware updater or
format-on-mount-failure behavior.

## Arduino CLI

After installing the same core and libraries, build with:

```powershell
arduino-cli compile --fqbn 'esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=fatflash,PSRAM=opi' mainCamera
```

Run the [hardware checks](../tests/README.md#hardware-checks-after-uploading)
after uploading. The [native checks](../tests/README.md#native-lvgl-check)
exercise the UI, decoders, battery filtering and diagnostics with simulated hardware.

## Bundled library provenance

Archives came from the [official Freenove library folder](https://github.com/Freenove/Freenove_Media_Kit_for_ESP32-S3/tree/main/Libraries).
`TFT_eSPI_Setups.zip` was adjusted to select the FNK0102A display profile.

| Archive | SHA-256 |
| --- | --- |
| `lvgl.zip` | `d687a7cec1466743e9d1b9c7f4f4b033a9121a17d8d5d0ae6461eeb284e84f20` |
| `TFT_eSPI.zip` | `b471fc7af1daecd8b022f8e2ec4ca3da8301564676c9e3b6986e51ff2762787b` |
| `TFT_eSPI_Setups.zip` | `05aae81c0afc69e1ab3ce5b5d4d30972d0c9343296c413f8866c28b612eb3fc4` |
| `Freenove_WS2812_Lib_for_ESP32.zip` | `9fe4003730d85ef5e9e8f2ed707b57825b62c36e1e1d53409c07feb2c756e7a2` |
