# Vortex: ESP32-2432S028 CYD AI Firmware

Vortex is an offline-first assistant for the ESP32-2432S028 Cheap Yellow Display (CYD). He knows that **Dzavious Ojelade is his maker**, shows a colorful TFT status UI, can connect to WiFi, and learns from files placed on the SD card.

## What Vortex Does

- Boots a CYD-friendly TFT UI with SD, WiFi, and local-model status.
- Connects to WiFi from `/vortex/wifi.txt` when credentials are present.
- Creates an editable SD-card layout on first boot:
  - `/vortex/purpose.txt` — Vortex's purpose and maker identity.
  - `/vortex/vortex_policy.txt` — editable purpose, whitelist, and blacklist.
  - `/vortex/knowledge/` — drag-and-drop files here so Vortex can learn printable snippets at boot.
  - `/vortex/models/` — drag-and-drop local model files here (`.tflite`, `.bin`, `.onnx`, `.gguf`) for future runtime model adapters.
  - `/vortex/wifi.txt` — editable WiFi credentials.
- Comprehends built-in topics including greetings, identity, creator, local files, local models, physics, math, AI, memory, energy, and computation.

## SD Card Setup

Format the card as FAT32. Vortex will create this structure automatically:

```text
/vortex/
  purpose.txt
  vortex_policy.txt
  wifi.txt
  ai_session.txt
  knowledge/
    your-notes.txt
    any-topic.md
    any-other-file
  models/
    your-local-model.tflite
```

Edit `/vortex/wifi.txt`:

```ini
ssid=YOUR_WIFI_NAME
password=YOUR_WIFI_PASSWORD
```

Edit `/vortex/vortex_policy.txt` any time you want to change Vortex's purpose, whitelist, or blacklist.

## Build & Flash

```bash
platformio run
platformio run --target upload
platformio device monitor --baud 115200
```

## Arduino IDE Build Option

If PlatformIO gives you `idf_tools.py installation failed`, you can use the self-contained Arduino IDE sketch in `arduino/vortex_cyd/`. Open `arduino/vortex_cyd/vortex_cyd.ino` in Arduino IDE, install the ESP32 board package plus `TFT_eSPI` and `ArduinoJson`, copy `arduino/vortex_cyd/TFT_eSPI_User_Setup.h` into the TFT_eSPI library setup, then click **Verify** and **Upload**. Full steps are in `arduino/vortex_cyd/README_ARDUINO_IDE.md`. If you see `TFT_eSPI.h: No such file or directory`, install `TFT_eSPI` from Arduino IDE Library Manager or use the sketch serial fallback until the display library is installed.

## Hardware Notes

The PlatformIO config targets `esp32dev` with TFT_eSPI pins for the common ESP32-2432S028 CYD ILI9341 display. SD uses CS GPIO 5 by default in firmware.
