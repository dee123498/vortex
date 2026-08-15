#include "ota_handler.h"

OTAUpdateHandler::OTAUpdateHandler() {}

String OTAUpdateHandler::getVersion() {
  return version;
}

bool OTAUpdateHandler::downloadAndUpdate(const String &updateURL) {
  (void)updateURL;
  Serial.println("[OTA] OTA update support is disabled in offline-safe build");
  return false;
}

bool OTAUpdateHandler::saveUpdateLog(const String &log) {
  (void)log;
  Serial.println("[OTA] OTA log disabled in offline-safe build");
  return true;
}
