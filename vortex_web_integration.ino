/* Vortex Wireless Web UI integration for ESP32 CYD */
#include <Arduino.h>
#include <SD.h>
#include "src/vortex_web.h"

extern bool sdOK;

// The model file is considered available when the SD card contains it.
// The actual model loader remains owned by the main Vortex firmware.
bool vortexModelReady(){
  return sdOK && SD.exists("/vortex/model.bin");
}

// ESP32 Arduino provides setup1/loop1 on the second core. This keeps the
// browser server responsive while Vortex performs inference on the main core.
void setup1(){
  delay(1200);
  vortexWebBegin();
}

void loop1(){
  vortexWebLoop();
  delay(2);
}
