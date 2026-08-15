#include <Arduino.h>
#include "tiny_llm.h"

TinyLLM llm;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== Vortex: Offline AI on ESP32 ===\n");

  String prompts[] = {
    "Hello, who are you?",
    "Tell me about physics",
    "Can you explain math?",
    "What is artificial intelligence?",
    "Tell me about memory management",
    "Explain energy efficiency",
    "How does computation work?",
    "Tell me a joke"
  };

  int numPrompts = sizeof(prompts) / sizeof(prompts[0]);
  for (int i = 0; i < numPrompts; ++i) {
    String p = prompts[i];
    Serial.println("---");
    Serial.println("Prompt: " + p);
    Serial.println("Vortex: " + llm.generate(p, 32));
  }

  Serial.println("\n=== Setup Complete ===\n");
}

void loop() {
  delay(10000);
  Serial.println("[LOOP] Vortex is running offline.");
}
