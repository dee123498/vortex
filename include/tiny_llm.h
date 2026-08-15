#pragma once
#include <Arduino.h>

class TinyLLM {
public:
  TinyLLM();
  String generate(const String &prompt, int maxTokens = 32);
private:
  int detectTopic(const String &prompt);
  String respondToTopic(int topic, const String &prompt);
};
