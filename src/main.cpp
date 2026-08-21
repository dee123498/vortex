#include <Arduino.h>
#include "tiny_llm.h"
#include "vortex_web.h"

TinyLLM llm;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== Vortex AI ===\n");

  vortexWebBegin();
  Serial.println("[VORTEX] Web dashboard and voice interface ready.");

  String prompts[] = {
    "Hello, who are you?",
    "Tell me about physics",
    "Can you explain math?"
  };
  for (auto &p : prompts) {
    Serial.println("---");
    Serial.println("Prompt: " + p);
    Serial.println("Vortex: " + llm.generate(p, 32));
  }
}

void loop() {
  vortexWebLoop();
  delay(2);
}
