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
#define TOPIC_UNKNOWN 0

TinyLLM::TinyLLM() {}

int TinyLLM::detectTopic(const String &prompt) {
  String p = prompt;
  p.toLowerCase();
  
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
      return "Hello! I'm Vortex, a tiny offline LLM. What can I help you with?";
    
    case TOPIC_IDENTITY:
      return "I'm Vortex, a quantized neural network running locally on ESP32 without internet.";
    
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
      return "Computation is my essence. I perform billions of operations per second in fixed-point arithmetic.";
    
    default:
      return "I'm Vortex. Tell me about physics, math, AI, memory, energy, or computation!";
  }
}

String TinyLLM::generate(const String &prompt, int maxTokens) {
  int topic = detectTopic(prompt);
  return respondToTopic(topic, prompt);
}
