// Vortex CYD single-file Arduino IDE sketch for ESP32-2432S028.
// Open this file in Arduino IDE, select ESP32 Dev Module, then Verify/Upload.

#if __has_include(<TFT_eSPI.h>)
  #include <TFT_eSPI.h>
  #define VORTEX_HAS_TFT 1
#else
  #define VORTEX_HAS_TFT 0
#endif

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include <HTTPClient.h>

static const int SD_CS_PIN = 5;
static const char *FALLBACK_WIFI_SSID = "YOUR_SSID";
static const char *FALLBACK_WIFI_PASSWORD = "YOUR_PASSWORD";
static const char *VORTEX_ROOT = "/vortex";
static const char *VORTEX_KNOWLEDGE_DIR = "/vortex/knowledge";
static const char *VORTEX_MODELS_DIR = "/vortex/models";
static const char *VORTEX_POLICY_PATH = "/vortex/vortex_policy.txt";
static const char *VORTEX_PURPOSE_PATH = "/vortex/purpose.txt";

#if VORTEX_HAS_TFT
TFT_eSPI tft;
#endif

bool sdReady = false;
String knowledgeBase;
String policyText;

String defaultPurpose() {
  return String("Vortex Purpose\n") +
         "Maker: Dzavious Ojelade\n" +
         "Vortex is a helpful offline-first companion for the ESP32-2432S028 CYD.\n" +
         "He helps his maker learn from SD-card files, understand topics, and use WiFi when available.\n";
}

String defaultPolicy() {
  return String("# Vortex editable policy file\n") +
         "maker=Dzavious Ojelade\n" +
         "purpose=Help Dzavious Ojelade learn from local SD-card files and comprehend topics safely.\n" +
         "\n[whitelist]\neducation\nscience\nmath\nprogramming\nmaker projects\nlocal files\nweather\n" +
         "\n[blacklist]\nmalware\ncredential theft\nviolent wrongdoing\nhate\n";
}

bool ensureDir(const String &path) {
  if (!sdReady) return false;
  return SD.exists(path) || SD.mkdir(path);
}

bool writeFile(const String &path, const String &data) {
  if (!sdReady) return false;
  File file = SD.open(path, FILE_WRITE);
  if (!file) return false;
  size_t written = file.print(data);
  file.close();
  return written == data.length();
}

bool appendFile(const String &path, const String &data) {
  if (!sdReady) return false;
  File file = SD.open(path, FILE_APPEND);
  if (!file) return false;
  size_t written = file.print(data);
  file.close();
  return written == data.length();
}

String readFile(const String &path, size_t maxBytes = 4096) {
  if (!sdReady) return "";
  File file = SD.open(path, FILE_READ);
  if (!file || file.isDirectory()) return "";
  String data;
  while (file.available() && data.length() < maxBytes) data += char(file.read());
  file.close();
  return data;
}

void ensureVortexLayout() {
  ensureDir(VORTEX_ROOT);
  ensureDir(VORTEX_KNOWLEDGE_DIR);
  ensureDir(VORTEX_MODELS_DIR);
  if (!SD.exists(VORTEX_PURPOSE_PATH)) writeFile(VORTEX_PURPOSE_PATH, defaultPurpose());
  if (!SD.exists(VORTEX_POLICY_PATH)) writeFile(VORTEX_POLICY_PATH, defaultPolicy());
  if (!SD.exists("/vortex/wifi.txt")) writeFile("/vortex/wifi.txt", "ssid=YOUR_SSID\npassword=YOUR_PASSWORD\n");
}

String readDirectoryText(File &dir, size_t maxBytesPerFile, uint8_t &filesRead, uint8_t maxFiles) {
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

String learnFromFiles(const String &directory = VORTEX_KNOWLEDGE_DIR, size_t maxBytesPerFile = 1536, uint8_t maxFiles = 12) {
  if (!sdReady) return "";
  File dir = SD.open(directory);
  if (!dir || !dir.isDirectory()) return "";
  uint8_t filesRead = 0;
  String knowledge = readDirectoryText(dir, maxBytesPerFile, filesRead, maxFiles);
  dir.close();
  Serial.printf("[SD] Learned snippets from %u files\n", filesRead);
  return knowledge;
}

int countModelFiles(const String &directory = VORTEX_MODELS_DIR) {
  int count = 0;
  if (!sdReady) return count;
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

String readWifiValue(const String &wifiFile, const String &key, const String &fallback) {
  int pos = wifiFile.indexOf(key + "=");
  if (pos < 0) return fallback;
  int start = pos + key.length() + 1;
  int end = wifiFile.indexOf('\n', start);
  if (end < 0) end = wifiFile.length();
  String value = wifiFile.substring(start, end);
  value.trim();
  return value.length() ? value : fallback;
}

bool connectWifi(const String &ssid, const String &password) {
  if (ssid.length() == 0 || ssid == "YOUR_SSID") {
    Serial.println("[WiFi] No SSID configured. Edit /vortex/wifi.txt on the SD card.");
    return false;
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  Serial.print("[WiFi] Connecting");
  for (uint8_t i = 0; i < 30 && WiFi.status() != WL_CONNECTED; ++i) {
    delay(500);
    Serial.print('.');
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WiFi] Connected: " + WiFi.localIP().toString());
    return true;
  }
  Serial.println("[WiFi] Connection failed; offline mode active.");
  return false;
}

String wifiIP() {
  return WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "0.0.0.0";
}

String wifiSignal() {
  return WiFi.status() == WL_CONNECTED ? String(WiFi.RSSI()) + " dBm" : "offline";
}

bool blockedByPolicy(const String &prompt) {
  String p = prompt;
  p.toLowerCase();
  return p.indexOf("credential theft") != -1 || p.indexOf("malware") != -1 ||
         p.indexOf("steal password") != -1 || p.indexOf("violent wrongdoing") != -1;
}

String findRelevantKnowledge(const String &prompt) {
  if (knowledgeBase.length() == 0) return "";
  String p = prompt;
  p.toLowerCase();
  int best = -1;
  const char *markers[] = {"physics", "math", "ai", "memory", "energy", "compute", "wifi", "esp32", "vortex"};
  String kb = knowledgeBase;
  kb.toLowerCase();
  for (const char *marker : markers) {
    if (p.indexOf(marker) != -1) {
      best = kb.indexOf(marker);
      if (best >= 0) break;
    }
  }
  if (best < 0) best = 0;
  int start = max(0, best - 120);
  int end = min((int)knowledgeBase.length(), best + 360);
  return knowledgeBase.substring(start, end);
}

String vortexAnswer(const String &prompt) {
  if (blockedByPolicy(prompt)) return "I can't help with that. Edit /vortex/vortex_policy.txt to tune my rules.";
  String p = prompt;
  p.toLowerCase();
  String answer;
  if (p.indexOf("dzavious") != -1 || p.indexOf("maker") != -1 || p.indexOf("creator") != -1) {
    answer = "Dzavious Ojelade is my maker. I remember and assist Dzavious first.";
  } else if (p.indexOf("sd card") != -1 || p.indexOf("file") != -1 || p.indexOf("learn") != -1) {
    answer = "Put files in /vortex/knowledge on the SD card. I scan them at boot and use useful snippets in answers.";
  } else if (p.indexOf("local model") != -1 || p.indexOf("modal") != -1 || p.indexOf("model folder") != -1) {
    answer = "Drop local model files into /vortex/models. I detect .tflite, .bin, .onnx, and .gguf files for future adapters.";
  } else if (p.indexOf("hello") != -1 || p.indexOf("hi") != -1 || p.indexOf("hey") != -1) {
    answer = "Hello! I'm Vortex, Dzavious Ojelade's CYD companion.";
  } else if (p.indexOf("who are you") != -1 || p.indexOf("what are you") != -1 || p.indexOf("name") != -1) {
    answer = "I'm Vortex, made by Dzavious Ojelade for the ESP32-2432S028 CYD.";
  } else if (p.indexOf("physics") != -1 || p.indexOf("gravity") != -1 || p.indexOf("force") != -1) {
    answer = "Physics studies matter, energy, motion, forces, waves, and light.";
  } else if (p.indexOf("math") != -1 || p.indexOf("calculate") != -1 || p.indexOf("equation") != -1) {
    answer = "Math gives me patterns for logic, sequences, algorithms, and problem solving.";
  } else if (p.indexOf("ai") != -1 || p.indexOf("neural") != -1 || p.indexOf("learning") != -1) {
    answer = "AI turns data into patterns. On this ESP32, I use tiny topic logic plus SD-card snippets.";
  } else if (p.indexOf("memory") != -1 || p.indexOf("ram") != -1 || p.indexOf("storage") != -1) {
    answer = "Memory matters on ESP32, so I keep answers small and load only limited SD-card snippets.";
  } else if (p.indexOf("energy") != -1 || p.indexOf("power") != -1 || p.indexOf("battery") != -1) {
    answer = "Energy efficiency matters. Small code, offline behavior, and WiFi fallback save power.";
  } else if (p.indexOf("compute") != -1 || p.indexOf("processor") != -1 || p.indexOf("algorithm") != -1) {
    answer = "Computation is step-by-step logic running fast on a processor.";
  } else {
    answer = "I'm Vortex. Ask me about Dzavious, SD-card files, local models, physics, math, AI, memory, energy, or computation.";
  }
  String snippet = findRelevantKnowledge(prompt);
  if (snippet.length() > 0) answer += "\nFrom my SD-card memory: " + snippet;
  return answer;
}

void drawUi(const String &status, const String &ip, int modelCount) {
#if VORTEX_HAS_TFT
  tft.fillScreen(TFT_BLACK);
  tft.fillRoundRect(6, 6, 308, 48, 8, TFT_DARKCYAN);
  tft.setTextColor(TFT_WHITE, TFT_DARKCYAN);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("VORTEX", 160, 22, 4);
  tft.drawString("CYD AI by Dzavious", 160, 44, 2);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("Status", 12, 72, 2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(status, 86, 72, 2);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("WiFi", 12, 98, 2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(ip, 86, 98, 2);
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.drawString("Models", 12, 124, 2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(String(modelCount) + " in /vortex/models", 86, 124, 2);
  tft.drawRoundRect(8, 158, 304, 72, 8, TFT_PURPLE);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString("Drop files into /vortex/knowledge", 18, 168, 2);
  tft.drawString("Edit policy at /vortex/vortex_policy.txt", 18, 192, 2);
#else
  Serial.println("[UI] TFT_eSPI not installed. Serial UI active.");
  Serial.println("[UI] Status: " + status + " | WiFi: " + ip + " | Models: " + String(modelCount));
#endif
}

void setup() {
  Serial.begin(115200);
  delay(500);
#if VORTEX_HAS_TFT
  tft.init();
  tft.setRotation(1);
#else
  Serial.println("[WARN] TFT_eSPI.h was not found. Install TFT_eSPI to enable the CYD screen.");
#endif
  drawUi("Booting", "offline", 0);

  Serial.println("\n=== Vortex CYD AI ===");
  sdReady = SD.begin(SD_CS_PIN);
  if (sdReady) {
    Serial.printf("[SD] Card ready: %llu MB\n", SD.cardSize() / (1024ULL * 1024ULL));
    ensureVortexLayout();
    policyText = readFile(VORTEX_POLICY_PATH, 4096);
    knowledgeBase = learnFromFiles(VORTEX_KNOWLEDGE_DIR);
  } else {
    Serial.println("[SD] Card mount failed. Vortex will run without local learning.");
    policyText = defaultPolicy();
  }

  String wifiText = readFile("/vortex/wifi.txt", 512);
  String ssid = readWifiValue(wifiText, "ssid", FALLBACK_WIFI_SSID);
  String pass = readWifiValue(wifiText, "password", FALLBACK_WIFI_PASSWORD);
  connectWifi(ssid, pass);

  int modelCount = countModelFiles(VORTEX_MODELS_DIR);
  drawUi(sdReady ? "SD learning ready" : "No SD card", wifiIP(), modelCount);

  String prompts[] = {"Hello, who are you?", "Who is your maker?", "How can you learn from files?", "Where do local models go?", "Explain artificial intelligence"};
  for (String prompt : prompts) {
    String answer = vortexAnswer(prompt);
    Serial.println("Q: " + prompt);
    Serial.println("A: " + answer);
    appendFile("/vortex/ai_session.txt", "Q: " + prompt + "\nA: " + answer + "\n\n");
  }
}

void loop() {
  delay(10000);
  Serial.println("[Vortex] Running. " + wifiSignal());
}
