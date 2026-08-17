#pragma once

#include <Arduino.h>
#include <vector>

struct VortexPolicy {
  String purpose;
  String maker;
  std::vector<String> whitelist;
  std::vector<String> blacklist;
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
