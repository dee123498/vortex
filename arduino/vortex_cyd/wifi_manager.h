#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

class WiFiManager {
public:
  WiFiManager();
  bool connect(const String &ssid, const String &password);
  bool isConnected();
  void disconnect();
  String getIP();
  String getSignalStrength();
};

class HTTPClient_Vortex {
public:
  HTTPClient_Vortex();
  String fetchWeather(const String &city);
  String webSearch(const String &query);
  String checkForUpdate(const String &updateURL);
private:
  HTTPClient http;
  String buildWeatherURL(const String &city);
  String buildSearchURL(const String &query);
};
