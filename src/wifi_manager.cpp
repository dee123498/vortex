#include "wifi_manager.h"

WiFiManager::WiFiManager() {}

bool WiFiManager::connect(const String &ssid, const String &password) {
  (void)ssid;
  (void)password;
  return false;
}

bool WiFiManager::isConnected() {
  return false;
}

void WiFiManager::disconnect() {}

String WiFiManager::getIP() {
  return "0.0.0.0";
}

String WiFiManager::getSignalStrength() {
  return "0 dBm";
}

HTTPClient_Vortex::HTTPClient_Vortex() {}

String HTTPClient_Vortex::buildWeatherURL(const String &city) {
  (void)city;
  return "";
}

String HTTPClient_Vortex::buildSearchURL(const String &query) {
  (void)query;
  return "";
}

String HTTPClient_Vortex::fetchWeather(const String &city) {
  (void)city;
  return "Weather API is disabled in offline-safe build";
}

String HTTPClient_Vortex::webSearch(const String &query) {
  (void)query;
  return "Search disabled in offline-safe build";
}

String HTTPClient_Vortex::checkForUpdate(const String &updateURL) {
  (void)updateURL;
  return "No update check in offline-safe build";
}
