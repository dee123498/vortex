# Vortex — ESP32 Offline AI Starter

This repository contains a starter project for "Vortex", an offline AI assistant targeted at the ESP32-2432S028 board.

Goals for this starter:
- ESP-IDF-based skeleton for audio capture, KWS (TensorFlow Lite Micro), simple intent handling, display UI (SPI), button control, SD card support, and over‑the‑air updates via Wi‑Fi.
- Provide build instructions, wiring notes, and pointers to add TinyML models and TTS assets.
- Push an initial branch `feat/vortex-esp32-starter` with a working skeleton (main app prints startup messages and demonstrates SD card detection and display placeholder). Full TinyML integration and heavy model files are left as placeholders and documented.

Board details (from user): ESP32-2432S028 (please confirm exact MCU variant and PSRAM/flash sizes). User provided: 128 GB SD card.

What you get in this repo:
- README with design and build instructions
- ESP-IDF project skeleton (CMakeLists, main app)
- Component placeholders and guidance for adding TensorFlow Lite Micro, display driver, and audio pipeline
- Example intent mapping JSON and scripts pointers for training/quantization

Next steps I will take after this commit:
1. Create branch `feat/vortex-esp32-starter` and push the starter project files (ESP-IDF skeleton, main.c, CMake files, components/README, and examples).  
2. After the branch is created and files are pushed, I will open a follow-up message describing how to build and flash, and where to add the model files and TTS assets to the repo.

Please confirm the MCU variant (ESP32-S3 / S2 / C3 / other) and whether PSRAM is present. If you want, I will also include a CI workflow that builds the project on push.

---

License: MIT
