# Bundled Arduino libraries

These ZIP files are committed with the project so a clean checkout does not
depend on libraries already installed on the developer's computer.

| Archive | Version | Purpose |
| --- | --- | --- |
| `lvgl.zip` | 8.4.0 | User interface |
| `TFT_eSPI.zip` | 2.5.43 | ST7789 display driver |
| `TFT_eSPI_Setups.zip` | FNK0102A profile | Freenove 1.14-inch, 135x240 display setup |
| `Freenove_WS2812_Lib_for_ESP32.zip` | 2.0.0 | Status LED |

The libraries came from the official
[Freenove Media Kit for ESP32-S3 repository](https://github.com/Freenove/Freenove_Media_Kit_for_ESP32-S3/tree/main/Libraries).
`TFT_eSPI_Setups.zip` differs only in selecting
`FNK0102A_1P14_135x240_ST7789` instead of Freenove's current 3.5-inch default.

## SHA-256 checksums

```text
9fe4003730d85ef5e9e8f2ed707b57825b62c36e1e1d53409c07feb2c756e7a2  Freenove_WS2812_Lib_for_ESP32.zip
d687a7cec1466743e9d1b9c7f4f4b033a9121a17d8d5d0ae6461eeb284e84f20  lvgl.zip
05aae81c0afc69e1ab3ce5b5d4d30972d0c9343296c413f8866c28b612eb3fc4  TFT_eSPI_Setups.zip
b471fc7af1daecd8b022f8e2ec4ca3da8301564676c9e3b6986e51ff2762787b  TFT_eSPI.zip
```

On Windows, run `install-arduino-libraries.ps1` from the repository root.
Alternatively, install each archive using Arduino IDE's **Sketch > Include
Library > Add .ZIP Library** command.
