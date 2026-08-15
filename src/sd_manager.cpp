#include "sd_manager.h"

SDCardManager::SDCardManager() {}

bool SDCardManager::init(int chipSelect) {
  (void)chipSelect;
  Serial.println("[SD] SD card support is disabled in offline-safe build");
  return false;
}

bool SDCardManager::writeFile(const String &filename, const String &data) {
  (void)filename; (void)data;
  return false;
}

String SDCardManager::readFile(const String &filename) {
  (void)filename;
  return "SD file read disabled";
}

bool SDCardManager::appendFile(const String &filename, const String &data) {
  (void)filename; (void)data;
  return false;
}

bool SDCardManager::deleteFile(const String &filename) {
  (void)filename;
  return false;
}

bool SDCardManager::fileExists(const String &filename) {
  (void)filename;
  return false;
}

void SDCardManager::listFiles() {
  Serial.println("[SD] No SD card files available in offline-safe build");
}

uint64_t SDCardManager::getStorageSpace() {
  return 0;
}
