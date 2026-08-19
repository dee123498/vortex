#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <DNSServer.h>

extern WebServer vortexWeb;
extern DNSServer vortexDNS;
extern String vortexWebAnswer(const String& message);
extern String vortexWebStatus();
extern String vortexWebPurpose();
extern void vortexWebHandleCommand(const String& message);
void vortexWebBegin();
void vortexWebLoop();
