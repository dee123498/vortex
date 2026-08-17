#pragma once

#include <Arduino.h>
struct VortexPolicy {
  String purpose;
  String maker;
  String whitelist;
  String blacklist;
};

class VortexConfig {
public:
  static const char *rootDir();
  static const char *knowledgeDir();
  static const char *modelsDir();
  static const char *configPath();
  static const char *purposePath();

  static String defaultPurpose();
  static String defaultConfig();
};
