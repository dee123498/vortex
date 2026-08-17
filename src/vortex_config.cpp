#include "vortex_config.h"

const char *VortexConfig::rootDir() { return "/vortex"; }
const char *VortexConfig::knowledgeDir() { return "/vortex/knowledge"; }
const char *VortexConfig::modelsDir() { return "/vortex/models"; }
const char *VortexConfig::configPath() { return "/vortex/vortex_policy.txt"; }
const char *VortexConfig::purposePath() { return "/vortex/purpose.txt"; }

String VortexConfig::defaultPurpose() {
  return String("Vortex Purpose\n") +
         "Maker: Dzavious Ojelade\n" +
         "Vortex is a helpful offline-first companion for the ESP32-2432S028 CYD.\n" +
         "He helps his maker learn, summarize SD-card files, understand topics, and use WiFi only when available.\n";
}

String VortexConfig::defaultConfig() {
  return String("# Vortex editable policy file\n") +
         "maker=Dzavious Ojelade\n" +
         "purpose=Help Dzavious Ojelade learn from local SD-card files and comprehend topics safely.\n" +
         "\n[whitelist]\n" +
         "education\nscience\nmath\nprogramming\nmaker projects\nlocal files\nweather\n" +
         "\n[blacklist]\n" +
         "malware\ncredential theft\nviolent wrongdoing\nhate\n" +
         "\n# Drop text notes into /vortex/knowledge and local model files into /vortex/models.\n";
}
