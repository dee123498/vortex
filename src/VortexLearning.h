#pragma once
#include <Arduino.h>
#include <SD.h>

class VortexLearning {
public:
  static constexpr uint8_t MAX_FILES = 48;
  static constexpr uint16_t MAX_FILE_NAME = 160;
  static constexpr uint16_t MAX_RESULT = 1800;

  struct Record {
    String path;
    uint32_t size = 0;
    uint32_t hash = 0;
    bool valid = false;
  };

  bool begin(bool sdReady);
  uint16_t scan();
  uint16_t fileCount() const { return count; }
  bool learnText(const String &text, const String &source = "conversation");
  bool forget(const String &text);
  bool remember(const String &text);
  String retrieve(const String &query, uint16_t maxChars = 1400);
  String status() const;
  bool reload();

private:
  bool ready = false;
  uint16_t count = 0;
  Record records[MAX_FILES];
  String normalize(String s) const;
  int scoreText(const String &query, const String &text) const;
  uint32_t fnv1a(const String &s) const;
  bool scanDir(const String &dir, uint8_t depth);
  void considerFile(const String &path);
  String readLimited(const String &path, uint16_t maxChars) const;
  String memoryPath() const { return "/vortex/memory/learned.txt"; }
};
