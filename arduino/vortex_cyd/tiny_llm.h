#pragma once
#include <Arduino.h>

class TinyLLM {
public:
  TinyLLM();
  void setKnowledgeBase(const String &knowledge);
  void setPolicy(const String &policyText);
  String generate(const String &prompt, int maxTokens = 32);
private:
  int detectTopic(const String &prompt);
  String respondToTopic(int topic, const String &prompt);
  String findRelevantKnowledge(const String &prompt);
  bool blockedByPolicy(const String &prompt);
  String knowledgeBase;
  String policy;
};
