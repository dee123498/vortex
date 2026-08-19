#include "VortexLearning.h"

String VortexLearning::normalize(String s) const {
  s.toLowerCase();
  String out;
  bool space = false;
  for (size_t i = 0; i < s.length(); ++i) {
    char c = s[i];
    if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
      out += c;
      space = false;
    } else if (!space) {
      out += ' ';
      space = true;
    }
  }
  out.trim();
  return out;
}

uint32_t VortexLearning::fnv1a(const String &s) const {
  uint32_t h = 2166136261UL;
  for (size_t i = 0; i < s.length(); ++i) {
    h ^= (uint8_t)s[i];
    h *= 16777619UL;
  }
  return h;
}

bool VortexLearning::begin(bool sdReady) {
  ready = sdReady;
  count = 0;
  if (!ready) return false;
  if (!SD.exists("/vortex")) SD.mkdir("/vortex");
  if (!SD.exists("/vortex/knowledge")) SD.mkdir("/vortex/knowledge");
  if (!SD.exists("/vortex/memory")) SD.mkdir("/vortex/memory");
  scan();
  return true;
}

void VortexLearning::considerFile(const String &path) {
  if (count >= MAX_FILES) return;
  String p = path;
  String low = p;
  low.toLowerCase();
  if (!(low.endsWith(".txt") || low.endsWith(".md") || low.endsWith(".json"))) return;
  File f = SD.open(p, FILE_READ);
  if (!f || f.isDirectory()) { if (f) f.close(); return; }
  records[count].path = p;
  records[count].size = f.size();
  f.close();
  String sample = readLimited(p, 900);
  records[count].hash = fnv1a(sample);
  records[count].valid = true;
  ++count;
}

bool VortexLearning::scanDir(const String &dir, uint8_t depth) {
  if (depth > 4 || count >= MAX_FILES) return true;
  File d = SD.open(dir, FILE_READ);
  if (!d || !d.isDirectory()) { if (d) d.close(); return false; }
  File f = d.openNextFile();
  while (f && count < MAX_FILES) {
    String p = String(f.name());
    bool isDir = f.isDirectory();
    f.close();
    if (isDir) {
      String low = p; low.toLowerCase();
      if (low != "/vortex/memory" || depth < 3) scanDir(p, depth + 1);
    } else {
      considerFile(p);
    }
    f = d.openNextFile();
  }
  d.close();
  return true;
}

uint16_t VortexLearning::scan() {
  if (!ready) return 0;
  count = 0;
  scanDir("/vortex/knowledge", 0);
  scanDir("/vortex/memory", 0);
  return count;
}

String VortexLearning::readLimited(const String &path, uint16_t maxChars) const {
  File f = SD.open(path, FILE_READ);
  if (!f || f.isDirectory()) { if (f) f.close(); return ""; }
  String s;
  while (f.available() && s.length() < maxChars) s += (char)f.read();
  f.close();
  return s;
}

int VortexLearning::scoreText(const String &query, const String &text) const {
  String q = normalize(query);
  String t = normalize(text);
  if (!q.length() || !t.length()) return 0;
  int score = 0;
  int start = 0;
  while (start < (int)q.length()) {
    int end = q.indexOf(' ', start);
    if (end < 0) end = q.length();
    String word = q.substring(start, end);
    if (word.length() >= 3 && t.indexOf(word) >= 0) score += 10;
    start = end + 1;
  }
  if (t.indexOf(q) >= 0) score += 40;
  return score;
}

String VortexLearning::retrieve(const String &query, uint16_t maxChars) {
  if (!ready || !query.length()) return "";
  int bestScore = 0;
  String best;
  String source;
  for (uint16_t i = 0; i < count; ++i) {
    if (!records[i].valid) continue;
    String text = readLimited(records[i].path, MAX_RESULT);
    int score = scoreText(query, text);
    if (score > bestScore) {
      bestScore = score;
      best = text;
      source = records[i].path;
    }
  }
  if (!best.length()) return "";
  best.trim();
  if (best.length() > maxChars) best = best.substring(0, maxChars);
  return "[SD: " + source + "]\n" + best;
}

bool VortexLearning::remember(const String &text) {
  if (!ready || !text.length()) return false;
  File f = SD.open(memoryPath(), FILE_APPEND);
  if (!f) f = SD.open(memoryPath(), FILE_WRITE);
  if (!f) return false;
  f.println(text);
  f.close();
  scan();
  return true;
}

bool VortexLearning::learnText(const String &text, const String &source) {
  if (!ready || !text.length()) return false;
  String clean = text;
  clean.replace("\r", " ");
  clean.replace("\n", " ");
  clean.trim();
  if (!clean.length()) return false;
  String line = "[" + source + "] " + clean;
  return remember(line);
}

bool VortexLearning::forget(const String &text) {
  if (!ready || !text.length()) return false;
  String all = readLimited(memoryPath(), 12000);
  if (!all.length()) return false;
  String needle = text;
  needle.trim();
  String rebuilt;
  int start = 0;
  bool removed = false;
  while (start < (int)all.length()) {
    int end = all.indexOf('\n', start);
    if (end < 0) end = all.length();
    String line = all.substring(start, end);
    if (line.indexOf(needle) < 0) {
      rebuilt += line;
      rebuilt += '\n';
    } else {
      removed = true;
    }
    start = end + 1;
  }
  if (!removed) return false;
  SD.remove(memoryPath());
  File f = SD.open(memoryPath(), FILE_WRITE);
  if (!f) return false;
  f.print(rebuilt);
  f.close();
  scan();
  return true;
}

bool VortexLearning::reload() {
  return ready && scan() >= 0;
}

String VortexLearning::status() const {
  if (!ready) return "Learning: SD unavailable";
  return "Learning: SD online\nIndexed files: " + String(count) + "\nMemory: /vortex/memory/learned.txt";
}
