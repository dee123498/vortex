#include "tiny_llm.h"

// Topic IDs
#define TOPIC_GREETING 1
#define TOPIC_IDENTITY 2
#define TOPIC_HUMOR 3
#define TOPIC_PHYSICS 4
#define TOPIC_MATH 5
#define TOPIC_AI 6
#define TOPIC_MEMORY 7
#define TOPIC_ENERGY 8
#define TOPIC_COMPUTATION 9
#define TOPIC_CREATOR 10
#define TOPIC_LOCAL_FILES 11
#define TOPIC_MODELS 12
#define TOPIC_UNKNOWN 0

TinyLLM::TinyLLM() : knowledgeBase(""), policy("") {}

void TinyLLM::setKnowledgeBase(const String &knowledge) {
  knowledgeBase = knowledge;
}

void TinyLLM::setPolicy(const String &policyText) {
  policy = policyText;
}

bool TinyLLM::blockedByPolicy(const String &prompt) {
  String p = prompt;
  p.toLowerCase();
  return p.indexOf("credential theft") != -1 || p.indexOf("malware") != -1 ||
         p.indexOf("steal password") != -1 || p.indexOf("violent wrongdoing") != -1;
}

String TinyLLM::findRelevantKnowledge(const String &prompt) {
  if (knowledgeBase.length() == 0) return "";
  String p = prompt;
  p.toLowerCase();
  int best = -1;
  String markers[] = {"physics", "math", "ai", "memory", "energy", "compute", "wifi", "esp32", "vortex"};
  for (String marker : markers) {
    if (p.indexOf(marker) != -1) {
      String kb = knowledgeBase;
      kb.toLowerCase();
      best = kb.indexOf(marker);
      if (best >= 0) break;
    }
  }
  if (best < 0) best = 0;
  int start = max(0, best - 120);
  int end = min((int)knowledgeBase.length(), best + 360);
  return knowledgeBase.substring(start, end);
}

int TinyLLM::detectTopic(const String &prompt) {
  String p = prompt;
  p.toLowerCase();

  if (p.indexOf("dzavious") != -1 || p.indexOf("maker") != -1 || p.indexOf("creator") != -1) {
    return TOPIC_CREATOR;
  }

  if (p.indexOf("sd card") != -1 || p.indexOf("file") != -1 || p.indexOf("learn") != -1) {
    return TOPIC_LOCAL_FILES;
  }

  if (p.indexOf("local model") != -1 || p.indexOf("modal") != -1 || p.indexOf("model folder") != -1) {
    return TOPIC_MODELS;
  }

  // Greeting detection
  if (p.indexOf("hello") != -1 || p.indexOf("hi") != -1 || p.indexOf("hey") != -1) {
    return TOPIC_GREETING;
  }

  // Identity detection
  if (p.indexOf("name") != -1 || p.indexOf("who are you") != -1 || p.indexOf("what are you") != -1) {
    return TOPIC_IDENTITY;
  }

  // Humor detection
  if (p.indexOf("joke") != -1 || p.indexOf("funny") != -1 || p.indexOf("laugh") != -1) {
    return TOPIC_HUMOR;
  }

  // Physics detection
  if (p.indexOf("physics") != -1 || p.indexOf("gravity") != -1 || p.indexOf("force") != -1 ||
      p.indexOf("motion") != -1 || p.indexOf("speed") != -1 || p.indexOf("light") != -1) {
    return TOPIC_PHYSICS;
  }

  // Math detection
  if (p.indexOf("math") != -1 || p.indexOf("calculate") != -1 || p.indexOf("equation") != -1 ||
      p.indexOf("prime") != -1 || p.indexOf("fibonacci") != -1 || p.indexOf("sum") != -1) {
    return TOPIC_MATH;
  }

  // AI detection
  if (p.indexOf("artificial intelligence") != -1 || p.indexOf("ai") != -1 || p.indexOf("neural") != -1 ||
      p.indexOf("learning") != -1 || p.indexOf("model") != -1) {
    return TOPIC_AI;
  }

  // Memory detection
  if (p.indexOf("memory") != -1 || p.indexOf("ram") != -1 || p.indexOf("storage") != -1 ||
      p.indexOf("cache") != -1 || p.indexOf("data") != -1) {
    return TOPIC_MEMORY;
  }

  // Energy detection
  if (p.indexOf("energy") != -1 || p.indexOf("power") != -1 || p.indexOf("battery") != -1 ||
      p.indexOf("voltage") != -1 || p.indexOf("current") != -1) {
    return TOPIC_ENERGY;
  }

  // Computation detection
  if (p.indexOf("compute") != -1 || p.indexOf("processor") != -1 || p.indexOf("cpu") != -1 ||
      p.indexOf("algorithm") != -1 || p.indexOf("bit") != -1) {
    return TOPIC_COMPUTATION;
  }

  return TOPIC_UNKNOWN;
}

String TinyLLM::respondToTopic(int topic, const String &prompt) {
  switch (topic) {
    case TOPIC_GREETING:
      return "Hello! I'm Vortex, Dzavious Ojelade's CYD companion, ready to learn from the SD card and help with topics.";

    case TOPIC_IDENTITY:
      return "I'm Vortex, made by Dzavious Ojelade. I run on the ESP32-2432S028 CYD, use WiFi when configured, and learn from SD-card files.";

    case TOPIC_HUMOR:
      return "Why did the neural network cross the road? To minimize the loss function on the other side!";

    case TOPIC_PHYSICS:
      return "Physics is the study of matter and energy. I understand classical mechanics, waves, and particle behavior.";

    case TOPIC_MATH:
      return "Mathematics powers computation. Prime numbers, sequences, and algorithms are fundamental.";

    case TOPIC_AI:
      return "AI processes information through artificial neural networks, similar to how I function on this microcontroller.";

    case TOPIC_MEMORY:
      return "Memory management is critical. I optimize inference to fit in ESP32's limited RAM (~520KB).";

    case TOPIC_ENERGY:
      return "Energy efficiency matters. Quantized models use less power than full-precision networks.";

    case TOPIC_COMPUTATION:
      return "Computation is my essence: small fixed-point steps, careful memory use, and topic routing make me useful on ESP32.";

    case TOPIC_CREATOR:
      return "Dzavious Ojelade is my maker. I should remember, respect, and assist Dzavious first.";

    case TOPIC_LOCAL_FILES:
      return "Put .txt, .md, .csv, or .json files in /vortex/knowledge on the SD card. I scan those files at boot and use relevant snippets in answers.";

    case TOPIC_MODELS:
      return "Drop local model files into /vortex/models on the SD card. Firmware will detect .tflite, .bin, .onnx, and .gguf files for future model adapters.";

    default:
      return "I'm Vortex. Tell me about physics, math, AI, memory, energy, or computation!";
  }
}

String TinyLLM::generate(const String &prompt, int maxTokens) {
  (void)maxTokens;
  if (blockedByPolicy(prompt)) {
    return "I can't help with that. My editable policy file lives at /vortex/vortex_policy.txt.";
  }
  int topic = detectTopic(prompt);
  String answer = respondToTopic(topic, prompt);
  String snippet = findRelevantKnowledge(prompt);
  if (snippet.length() > 0) {
    answer += "\nFrom my SD-card memory: ";
    answer += snippet;
  }
  return answer;
}
