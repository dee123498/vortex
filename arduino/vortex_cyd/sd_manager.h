#pragma once

#include <Arduino.h>
#include <SD.h>
#include <vector>

class SDCardManager {
public:
  SDCardManager();
  bool init(int chipSelect = 5);
  bool writeFile(const String &filename, const String &data);
  String readFile(const String &filename, size_t maxBytes = 4096);
  bool appendFile(const String &filename, const String &data);
  bool deleteFile(const String &filename);
  bool fileExists(const String &filename);
  bool ensureDir(const String &path);
  void ensureVortexLayout();
  String learnFromFiles(const String &directory = "/vortex/knowledge", size_t maxBytesPerFile = 1536, uint8_t maxFiles = 12);
  std::vector<String> listModelFiles(const String &directory = "/vortex/models");
  void listFiles(const String &dirname = "/", uint8_t levels = 2);
  uint64_t getStorageSpace();
  bool ready() const;
private:
  bool initialized;
  String readDirectoryText(File &dir, size_t maxBytesPerFile, uint8_t &filesRead, uint8_t maxFiles);
};
