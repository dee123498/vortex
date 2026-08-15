#pragma once

#include <Arduino.h>
#include <Update.h>
#include <HTTPClient.h>

class OTAUpdateHandler {
public:
  OTAUpdateHandler();
  bool downloadAndUpdate(const String &updateURL);
  String getVersion();
  bool saveUpdateLog(const String &log);
private:
  String version = "1.0.0";
};
