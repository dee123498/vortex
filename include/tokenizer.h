#pragma once

#include <Arduino.h>
#include <vector>

class Tokenizer {
public:
  Tokenizer();
  std::vector<int> encode(const String &text);
  String decode(const std::vector<int> &tokens);
private:
  int tokenFor(const String &word);
  String wordFor(int token);
};
