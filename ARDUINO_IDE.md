# Vortex 2.0 — Arduino IDE package

This branch is arranged so the Arduino IDE has **one sketch source**: `vortex2.ino`.

## Important

Use an **ESP32-S3**, not Arduino Uno.

Recommended Arduino IDE board:

`Tools -> Board -> esp32 -> ESP32S3 Dev Module`

Recommended settings:

- USB CDC On Boot: Enabled
- PSRAM: OPI PSRAM (if shown by your ESP32 core/board)
- Flash Size: 8MB
- Partition Scheme: Default 8MB
- Upload Mode: UART/USB according to your CYD board

## Libraries

Install these from Arduino IDE Library Manager:

1. ArduinoJson 7.x
2. TFT_eSPI 2.5.x

`XPT2046_Touchscreen` is provided locally as `XPT2046_Touchscreen.h` in the sketch folder. Do **not** install another XPT2046 library for this Vortex package.

## TFT_eSPI setup

The CYD display uses an ILI9341 controller. Configure TFT_eSPI for the CYD wiring used by this firmware:

- MOSI = GPIO 13
- SCLK = GPIO 14
- CS = GPIO 15
- DC = GPIO 2
- RST = -1
- BL = GPIO 21
- Width = 240
- Height = 320
- ILI9341 driver

The PlatformIO configuration already supplies these settings. Arduino IDE users must apply the equivalent TFT_eSPI `User_Setup` settings in their installed TFT_eSPI library.

## SD card

Create:

```text
/vortex/
  model.bin
  tokenizer.bin
  wifi.txt
  config.txt
  /knowledge/
    notes.txt
    lore.txt
```

`wifi.txt`:

```text
YOUR_WIFI_NAME
YOUR_WIFI_PASSWORD
```

`config.txt` example:

```text
location=Kankakee
latitude=41.1170
longitude=-88.2217
fahrenheit=1
```

No AI API key is required. Weather uses Open-Meteo when Wi-Fi is available.

## Why the duplicate-error problem was fixed

The previous branch contained both `vortex2.ino` and `src/main.ino`. Both contained Arduino `setup()`/`loop()` implementations, which could lead to duplicate-definition errors when files were copied into the same Arduino sketch folder.

`src/main.ino` has been removed from this branch. Keep the sketch folder limited to the single `vortex2.ino` source plus its local header.

## Arduino sketch folder

```text
Vortex2/
  vortex2.ino
  XPT2046_Touchscreen.h
  ARDUINO_IDE.md
```

Open `vortex2.ino` in Arduino IDE. Do not copy other `.ino` files into the same folder.
