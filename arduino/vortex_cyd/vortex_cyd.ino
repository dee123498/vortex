// Vortex CYD Arduino IDE sketch. Open this folder in Arduino IDE and upload to ESP32 Dev Module.
#if __has_include(<TFT_eSPI.h>)
  #include <TFT_eSPI.h>
  #define VORTEX_HAS_TFT 1
#else
  #define VORTEX_HAS_TFT 0
#endif
#include "sd_manager.h"
#include "tiny_llm.h"
#include "vortex_config.h"
#include "wifi_manager.h"

static const int SD_CS_PIN = 5;
static const char *FALLBACK_WIFI_SSID = "YOUR_SSID";
static const char *FALLBACK_WIFI_PASSWORD = "YOUR_PASSWORD";

#if VORTEX_HAS_TFT
TFT_eSPI tft;
#endif
TinyLLM llm;
SDCardManager sd;
WiFiManager wifi;

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
  Serial.println("[UI] TFT_eSPI is not installed, so the screen UI is disabled.");
  Serial.println("[UI] Install TFT_eSPI in Arduino IDE Library Manager for the CYD display.");
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
  Serial.println("[WARN] TFT_eSPI.h was not found at compile time. Install TFT_eSPI to enable the CYD screen.");
#endif
  drawUi("Booting", "offline", 0);

  Serial.println("\n=== Vortex CYD AI ===");
  bool sdReady = sd.init(SD_CS_PIN);
  String policy = sdReady ? sd.readFile(VortexConfig::configPath(), 4096) : VortexConfig::defaultConfig();
  String knowledge = sdReady ? sd.learnFromFiles(VortexConfig::knowledgeDir()) : "";
  llm.setPolicy(policy);
  llm.setKnowledgeBase(knowledge);

  String wifiText = sdReady ? sd.readFile("/vortex/wifi.txt", 512) : "";
  if (sdReady && !sd.fileExists("/vortex/wifi.txt")) {
    sd.writeFile("/vortex/wifi.txt", "ssid=YOUR_SSID\npassword=YOUR_PASSWORD\n");
  }
  String ssid = readWifiValue(wifiText, "ssid", FALLBACK_WIFI_SSID);
  String pass = readWifiValue(wifiText, "password", FALLBACK_WIFI_PASSWORD);
  wifi.connect(ssid, pass);

  int modelCount = sdReady ? sd.countModelFiles(VortexConfig::modelsDir()) : 0;
  drawUi(sdReady ? "SD learning ready" : "No SD card", wifi.getIP(), modelCount);

  String prompts[] = {"Hello, who are you?", "Who is your maker?", "How can you learn from files?", "Where do local models go?", "Explain artificial intelligence"};
  for (String prompt : prompts) {
    String answer = llm.generate(prompt, 64);
    Serial.println("Q: " + prompt);
    Serial.println("A: " + answer);
    if (sdReady) sd.appendFile("/vortex/ai_session.txt", "Q: " + prompt + "\nA: " + answer + "\n\n");
  }
}

void loop() {
  delay(10000);
  Serial.println("[Vortex] Running. " + wifi.getSignalStrength());
}
