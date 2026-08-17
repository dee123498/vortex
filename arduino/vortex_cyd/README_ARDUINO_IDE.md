# Vortex CYD - Arduino IDE Build

This folder is a self-contained Arduino IDE sketch for the ESP32-2432S028 CYD.

## 1. Install Arduino IDE

Install Arduino IDE 2.x, then open **File > Preferences** and add this Boards Manager URL:

```text
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

Go to **Tools > Board > Boards Manager**, search for **esp32**, and install **esp32 by Espressif Systems**.

## 2. Install libraries

Open **Tools > Manage Libraries** and install:

- `TFT_eSPI` by Bodmer
- `ArduinoJson` by Benoit Blanchon

`WiFi`, `HTTPClient`, and `SD` come with the ESP32 Arduino board package.

## 3. Configure TFT_eSPI for the CYD

Arduino IDE does not read PlatformIO `build_flags`, so you must configure TFT_eSPI once:

1. Find your Arduino libraries folder.
2. Open `TFT_eSPI/User_Setup.h`.
3. Replace its contents with the contents of `TFT_eSPI_User_Setup.h` from this sketch folder.
4. Save the file and restart Arduino IDE.

## 4. Open and upload

Open this file in Arduino IDE:

```text
arduino/vortex_cyd/vortex_cyd.ino
```

Use these board settings:

- Board: **ESP32 Dev Module**
- Upload Speed: **921600** or **115200** if upload fails
- Flash Mode: **DIO**
- Flash Frequency: **40MHz**
- Partition Scheme: **Default 4MB with spiffs** or similar
- Port: your ESP32 USB port

Click **Verify** to build, then **Upload** to flash.

If upload hangs on `Connecting...`, hold the CYD **BOOT** button until upload starts, then release it.

## 5. SD card setup

Format the SD card as FAT32. Vortex creates this layout on first boot:

```text
/vortex/
  purpose.txt
  vortex_policy.txt
  wifi.txt
  ai_session.txt
  knowledge/
  models/
```

Edit `/vortex/wifi.txt` on the SD card:

```ini
ssid=YOUR_WIFI_NAME
password=YOUR_WIFI_PASSWORD
```

Drop learning files into `/vortex/knowledge` and local model files into `/vortex/models`.


## Fix: `TFT_eSPI.h: No such file or directory`

That error means Arduino IDE cannot find the `TFT_eSPI` library. Open **Tools > Manage Libraries**, search for `TFT_eSPI`, install **TFT_eSPI by Bodmer**, then restart Arduino IDE and compile again.

The sketch now has a serial-only fallback so it can compile without the TFT library, but the CYD screen UI requires `TFT_eSPI` to be installed and configured with `TFT_eSPI_User_Setup.h`.


## Fix: `vector: No such file or directory`

Make sure **Tools > Board** is set to an ESP32 board such as **ESP32 Dev Module**. This sketch no longer depends on the C++ `<vector>` header, so if you still see this error you are probably compiling an old copy of the sketch or compiling for a non-ESP32 board.
