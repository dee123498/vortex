#include "tokenizer.h"
#include <map>

static std::map<String, int> vocab = {
  {"hello", 1},
  {"hi", 2},
  {"vortex", 3},
  {"physics", 4},
  {"math", 5},
  {"ai", 6},
  {"memory", 7},
  {"energy", 8},
  {"compute", 9},
  {"tell", 10},
  {"me", 11},
  {"joke", 12},
  {"name", 13},
  {"what", 14},
  {"is", 15},
  {"your", 16},
  {"gravity", 17},
  {"force", 18},
  {"neural", 19},
  {"network", 20},
  {"learning", 21},
  {"power", 22},
  {"battery", 23},
  {"algorithm", 24},
  {"data", 25},
  {"about", 26}
};

static std::map<int, String> ivocab;

Tokenizer::Tokenizer() {
  if (ivocab.empty()) {
    for (auto &p : vocab) ivocab[p.second] = p.first;
  }
}

int Tokenizer::tokenFor(const String &word) {
  auto it = vocab.find(word);
  if (it != vocab.end()) return it->second;
  return 0; // unknown
}

String Tokenizer::wordFor(int token) {
  auto it = ivocab.find(token);
  if (it != ivocab.end()) return it->second;
  return "<unk>";
}

std::vector<int> Tokenizer::encode(const String &text) {
  std::vector<int> out;
  String s = text;
  s.toLowerCase();
  int start = 0;
  for (int i = 0; i <= s.length(); ++i) {
    char c = i < s.length() ? s[i] : ' ';
    if (c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '.' || c == ',' || c == '?' || c == '!') {
      if (i - start > 0) {
        String token = s.substring(start, i);
        int id = tokenFor(token);
        out.push_back(id);
      }
      start = i + 1;
    }
  }
  return out;
}

String Tokenizer::decode(const std::vector<int> &tokens) {
  String out = "";
  for (size_t i = 0; i < tokens.size(); ++i) {
    if (i) out += " ";
    out += wordFor(tokens[i]);
  }
  return out;
}
