#include "wifi_manager.h"

WiFiManager::WiFiManager() {}

bool WiFiManager::connect(const String &ssid, const String &password) {
  if (ssid.length() == 0 || ssid == "YOUR_SSID") {
    Serial.println("[WiFi] No SSID configured. Edit /vortex/wifi.txt or firmware constants.");
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

bool WiFiManager::isConnected() { return WiFi.status() == WL_CONNECTED; }
void WiFiManager::disconnect() { WiFi.disconnect(true); }
String WiFiManager::getIP() { return isConnected() ? WiFi.localIP().toString() : "0.0.0.0"; }
String WiFiManager::getSignalStrength() { return isConnected() ? String(WiFi.RSSI()) + " dBm" : "offline"; }

HTTPClient_Vortex::HTTPClient_Vortex() {}

String HTTPClient_Vortex::buildWeatherURL(const String &city) {
  (void)city;
  return "http://api.open-meteo.com/v1/forecast?latitude=40.7128&longitude=-74.0060&current_weather=true";
}

String HTTPClient_Vortex::buildSearchURL(const String &query) {
  String encoded = query;
  encoded.replace(" ", "+");
  return "http://api.duckduckgo.com/?q=" + encoded + "&format=json&no_redirect=1";
}

String HTTPClient_Vortex::fetchWeather(const String &city) {
  if (WiFi.status() != WL_CONNECTED) return "Weather unavailable: WiFi offline.";
  http.begin(buildWeatherURL(city));
  int code = http.GET();
  String payload = code > 0 ? http.getString() : "Weather request failed.";
  http.end();
  return payload;
}

String HTTPClient_Vortex::webSearch(const String &query) {
  if (WiFi.status() != WL_CONNECTED) return "Search unavailable: WiFi offline.";
  http.begin(buildSearchURL(query));
  int code = http.GET();
  String payload = code > 0 ? http.getString() : "Search request failed.";
  http.end();
  return payload;
}

String HTTPClient_Vortex::checkForUpdate(const String &updateURL) {
  if (WiFi.status() != WL_CONNECTED) return "Update check unavailable: WiFi offline.";
  http.begin(updateURL);
  int code = http.GET();
  String payload = code > 0 ? http.getString() : "Update check failed.";
  http.end();
  return payload;
}
