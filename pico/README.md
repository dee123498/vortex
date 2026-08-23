# Vortex Pico AI Coprocessor

Target: Raspberry Pi Pico (RP2040), Arduino framework.

## UART to CYD

- GP4 / physical pin 6: TX -> CYD GPIO3 RX
- GP5 / physical pin 7: RX <- CYD GPIO1 TX
- GND: common ground
- 115200 baud, 8N1

The Pico owns the Vortex conversational state, selected-user identity, short-term per-user memory, intent scoring, and the offline response engine.

## Important model limitation

The RP2040 has 264 KB RAM and 2 MB flash. A normal desktop LLM or the CYD SD-card `model.bin` cannot simply be loaded into the Pico. The Pico firmware therefore does **not** pretend to run the large SD model. It provides a reliable offline AI coprocessor that works immediately and leaves a clean protocol for a future tiny quantized RP2040 model backend.

The CYD can still store `/vortex/model.bin` and `/vortex/tokenizer.bin` for the ESP32-side model work; the Pico does not read the CYD SD card directly.

## USB test

Open the Pico serial monitor at 115200 and type a line. The Pico forwards it as a prompt to the CYD link and returns the AI response on the CYD UART.
