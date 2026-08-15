#pragma once

#include <Arduino.h>
#include <SD.h>

class SDCardManager {
public:
  SDCardManager();
  bool init(int chipSelect = 5);
  bool writeFile(const String &filename, const String &data);
  String readFile(const String &filename);
  bool appendFile(const String &filename, const String &data);
  bool deleteFile(const String &filename);
  bool fileExists(const String &filename);
  void listFiles();
  uint64_t getStorageSpace();
};
