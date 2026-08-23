#pragma once
#include <Arduino.h>

// Browser dashboard hosted directly by the CYD ESP32.
// The dashboard uses the same Pico UART already connected to GPIO1/GPIO3.
void vortexWebBegin();
void vortexWebLoop();
void vortexWebOnPicoLine(const String &line);
