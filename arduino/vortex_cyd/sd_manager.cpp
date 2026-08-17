#include "sd_manager.h"
#include "vortex_config.h"

SDCardManager::SDCardManager() : initialized(false) {}

bool SDCardManager::init(int chipSelect) {
  initialized = SD.begin(chipSelect);
  if (!initialized) {
    Serial.println("[SD] Card mount failed. Vortex will continue without local learning.");
    return false;
  }
  Serial.printf("[SD] Card ready: %llu MB\n", SD.cardSize() / (1024ULL * 1024ULL));
  ensureVortexLayout();
  return true;
}

bool SDCardManager::ready() const { return initialized; }

bool SDCardManager::ensureDir(const String &path) {
  if (!initialized) return false;
  if (SD.exists(path)) return true;
  bool ok = SD.mkdir(path);
  Serial.println(ok ? "[SD] Created " + path : "[SD] Failed to create " + path);
  return ok;
}

void SDCardManager::ensureVortexLayout() {
  ensureDir(VortexConfig::rootDir());
  ensureDir(VortexConfig::knowledgeDir());
  ensureDir(VortexConfig::modelsDir());
  if (!fileExists(VortexConfig::purposePath())) writeFile(VortexConfig::purposePath(), VortexConfig::defaultPurpose());
  if (!fileExists(VortexConfig::configPath())) writeFile(VortexConfig::configPath(), VortexConfig::defaultConfig());
}

bool SDCardManager::writeFile(const String &filename, const String &data) {
  if (!initialized) return false;
  File file = SD.open(filename, FILE_WRITE);
  if (!file) return false;
  size_t written = file.print(data);
  file.close();
  return written == data.length();
}

String SDCardManager::readFile(const String &filename, size_t maxBytes) {
  if (!initialized) return "";
  File file = SD.open(filename, FILE_READ);
  if (!file || file.isDirectory()) return "";
  String data;
  while (file.available() && data.length() < maxBytes) data += char(file.read());
  file.close();
  return data;
}

bool SDCardManager::appendFile(const String &filename, const String &data) {
  if (!initialized) return false;
  File file = SD.open(filename, FILE_APPEND);
  if (!file) return false;
  size_t written = file.print(data);
  file.close();
  return written == data.length();
}

bool SDCardManager::deleteFile(const String &filename) {
  return initialized && SD.remove(filename);
}

bool SDCardManager::fileExists(const String &filename) {
  return initialized && SD.exists(filename);
}

String SDCardManager::readDirectoryText(File &dir, size_t maxBytesPerFile, uint8_t &filesRead, uint8_t maxFiles) {
  String knowledge;
  while (filesRead < maxFiles) {
    File entry = dir.openNextFile();
    if (!entry) break;
    String name = entry.name();
    if (entry.isDirectory()) {
      knowledge += readDirectoryText(entry, maxBytesPerFile, filesRead, maxFiles);
    } else {
      knowledge += "\n[File: " + name + "]\n";
      size_t read = 0;
      while (entry.available() && read++ < maxBytesPerFile) {
        char c = char(entry.read());
        knowledge += ((c >= 32 && c <= 126) || c == '\n' || c == '\r' || c == '\t') ? c : ' ';
      }
      knowledge += "\n";
      filesRead++;
    }
    entry.close();
  }
  return knowledge;
}

String SDCardManager::learnFromFiles(const String &directory, size_t maxBytesPerFile, uint8_t maxFiles) {
  if (!initialized) return "";
  File dir = SD.open(directory);
  if (!dir || !dir.isDirectory()) return "";
  uint8_t filesRead = 0;
  String knowledge = readDirectoryText(dir, maxBytesPerFile, filesRead, maxFiles);
  dir.close();
  Serial.printf("[SD] Learned snippets from %u files\n", filesRead);
  return knowledge;
}

int SDCardManager::countModelFiles(const String &directory) {
  int count = 0;
  if (!initialized) return count;
  File dir = SD.open(directory);
  if (!dir || !dir.isDirectory()) return count;
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;
    String name = entry.name();
    if (!entry.isDirectory() && (name.endsWith(".tflite") || name.endsWith(".bin") || name.endsWith(".onnx") || name.endsWith(".gguf"))) {
      count++;
      Serial.println("[SD] Local model found: " + name);
    }
    entry.close();
  }
  dir.close();
  return count;
}

void SDCardManager::listFiles(const String &dirname, uint8_t levels) {
  if (!initialized) return;
  File root = SD.open(dirname);
  if (!root) return;
  while (true) {
    File file = root.openNextFile();
    if (!file) break;
    Serial.printf(file.isDirectory() ? "[DIR] %s\n" : "[FILE] %s (%u bytes)\n", file.name(), file.size());
    if (file.isDirectory() && levels) listFiles(String(file.name()), levels - 1);
    file.close();
  }
}

uint64_t SDCardManager::getStorageSpace() { return initialized ? SD.cardSize() : 0; }
