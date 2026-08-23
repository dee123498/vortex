#include <Arduino.h>

/*
  VORTEX AI COPROCESSOR - Raspberry Pi Pico
  UART1:
    GP4 (physical pin 6) = TX -> CYD RX (GPIO3)
    GP5 (physical pin 7) = RX <- CYD TX (GPIO1)
    GND                  -> CYD GND

  The Pico owns conversational state, user context, intent scoring and the
  offline response engine. The CYD owns display, touch, SD storage and Wi-Fi.
*/

static constexpr uint32_t BAUD = 115200;
static constexpr uint8_t CYD_TX_PIN = 4; // GP4
static constexpr uint8_t CYD_RX_PIN = 5; // GP5

struct UserProfile {
  String id;
  String name;
  String memory[4];
  uint8_t memoryCount = 0;
  uint32_t messages = 0;
};

UserProfile users[6];
uint8_t userCount = 0;
int activeUser = -1;
String rxLine;

void addUser(const String &id, const String &name) {
  for (uint8_t i = 0; i < userCount; ++i) {
    if (users[i].id == id) {
      users[i].name = name.length() ? name : id;
      activeUser = i;
      return;
    }
  }
  if (userCount >= 6) return;
  users[userCount].id = id;
  users[userCount].name = name.length() ? name : id;
  users[userCount].memoryCount = 0;
  users[userCount].messages = 0;
  activeUser = userCount++;
}

void remember(const String &text) {
  if (activeUser < 0 || !text.length()) return;
  String t = text;
  t.trim();
  if (t.length() > 80) t = t.substring(0, 80);
  String low = t;
  low.toLowerCase();
  if (low.indexOf("hello") == 0 || low.indexOf("hi ") == 0) return;

  if (users[activeUser].memoryCount < 4) {
    users[activeUser].memory[users[activeUser].memoryCount++] = t;
  } else {
    for (int i = 0; i < 3; ++i) users[activeUser].memory[i] = users[activeUser].memory[i + 1];
    users[activeUser].memory[3] = t;
  }
}

String activeName() {
  return activeUser >= 0 ? users[activeUser].name : String("User");
}

bool hasAny(const String &s, const char *a, const char *b = nullptr,
            const char *c = nullptr, const char *d = nullptr) {
  return s.indexOf(a) >= 0 ||
         (b && s.indexOf(b) >= 0) ||
         (c && s.indexOf(c) >= 0) ||
         (d && s.indexOf(d) >= 0);
}

String answerFor(const String &input) {
  String p = input;
  p.trim();
  String low = p;
  low.toLowerCase();

  if (low.length() == 0) return "I'm listening.";

  if (hasAny(low, "who are you", "what are you", "your name"))
    return "I'm Vortex, your local AI coprocessor. I'm running on the Raspberry Pi Pico.";

  if (hasAny(low, "hello", "hi", "hey", "yo "))
    return "Hello, " + activeName() + ". I'm ready.";

  if (hasAny(low, "who am i", "do you know me", "remember me")) {
    if (activeUser < 0) return "I know you as an unidentified user. Choose a user profile on the CYD.";
    String r = "You're " + activeName() + ".";
    if (users[activeUser].memoryCount)
      r += " I remember: " + users[activeUser].memory[users[activeUser].memoryCount - 1];
    return r;
  }

  if (hasAny(low, "what can you do", "help", "capabilities"))
    return "I can chat offline, keep short-term memory per user, answer common questions, do simple arithmetic, and control the CYD through commands.";

  if (hasAny(low, "how are you", "are you okay"))
    return "I'm running normally. Pico AI core is online.";

  if (hasAny(low, "ai", "artificial intelligence", "neural", "model"))
    return "I am the AI coprocessor. The CYD provides my screen, touch controls, SD storage, and Wi-Fi services.";

  if (hasAny(low, "memory", "remember", "forget")) {
    if (low.indexOf("forget") >= 0 && activeUser >= 0) {
      users[activeUser].memoryCount = 0;
      return "I cleared this user's short-term memory.";
    }
    if (activeUser >= 0 && users[activeUser].memoryCount)
      return "My short-term memory for " + activeName() + " contains " + String(users[activeUser].memoryCount) + " recent item(s).";
    return "I can keep a few short-term notes for each selected user.";
  }

  if (hasAny(low, "joke", "funny", "laugh"))
    return "Why did the microcontroller get promoted? It finally had enough processing power to handle the responsibility.";

  if (hasAny(low, "weather", "temperature"))
    return "The CYD can check weather over Wi-Fi. Ask the CYD for the current weather.";

  if (hasAny(low, "physics", "gravity", "force", "motion"))
    return "Physics describes matter, motion, energy, and the forces that connect them.";

  if (hasAny(low, "math", "calculate", "equation"))
    return "I can handle simple arithmetic. Send an expression such as 12+7, 8*9, or 100/4.";

  if (hasAny(low, "goodbye", "bye"))
    return "Goodbye, " + activeName() + ". I'll be here when you return.";

  // Lightweight arithmetic: one operator and two numeric values.
  int op = -1;
  char oper = 0;
  for (int i = 1; i < (int)low.length() - 1; ++i) {
    if (low[i] == '+' || low[i] == '-' || low[i] == '*' || low[i] == '/') {
      op = i;
      oper = low[i];
      break;
    }
  }
  if (op > 0) {
    String a = low.substring(0, op);
    String b = low.substring(op + 1);
    a.trim(); b.trim();
    bool okA = a.length() && (a.toFloat() != 0.0f || a == "0");
    bool okB = b.length() && (b.toFloat() != 0.0f || b == "0");
    if (okA && okB) {
      float x = a.toFloat(), y = b.toFloat(), z = 0;
      if (oper == '+') z = x + y;
      else if (oper == '-') z = x - y;
      else if (oper == '*') z = x * y;
      else {
        if (fabsf(y) < 0.000001f) return "I can't divide by zero.";
        z = x / y;
      }
      return "The answer is " + String(z, 4);
    }
  }

  if (activeUser >= 0)
    return "I heard you, " + activeName() + ". I can work with that, but this offline Pico build does not have a full-size language model.";
  return "I heard you. Select a user profile for personalized memory.";
}

void sendFrame(const String &text) {
  String safe = text;
  safe.replace("\r", " ");
  safe.replace("\n", " ");
  Serial1.println("TEXT\t" + safe);
}

void handleCommand(String line) {
  line.trim();
  if (!line.length()) return;

  int tab = line.indexOf('\t');
  String cmd = tab >= 0 ? line.substring(0, tab) : line;
  String payload = tab >= 0 ? line.substring(tab + 1) : "";

  if (cmd == "PING") {
    Serial1.println("PONG\tPICO_AI\t1");
    return;
  }
  if (cmd == "HELLO") {
    Serial1.println("READY\tPICO_AI\tVORTEX");
    return;
  }
  if (cmd == "USER") {
    int t = payload.indexOf('\t');
    String id = t >= 0 ? payload.substring(0, t) : payload;
    String name = t >= 0 ? payload.substring(t + 1) : id;
    id.trim(); name.trim();
    if (!id.length()) id = "guest";
    addUser(id, name);
    Serial1.println("USER_OK\t" + id + "\t" + activeName());
    return;
  }
  if (cmd == "RESET") {
    rxLine = "";
    Serial1.println("RESET_OK");
    return;
  }
  if (cmd == "PROMPT") {
    String response = answerFor(payload);
    remember(payload);
    if (activeUser >= 0) users[activeUser].messages++;
    Serial1.println("BEGIN");
    sendFrame(response);
    Serial1.println("END");
    return;
  }
  if (cmd == "STATUS") {
    Serial1.println("STATUS\tPICO_AI\tONLINE\tUSER=" + activeName());
    return;
  }
  Serial1.println("ERROR\tUNKNOWN_COMMAND");
}

void readPort(Stream &port, bool fromUsb) {
  while (port.available()) {
    char c = (char)port.read();
    if (c == '\r') continue;
    if (c == '\n') {
      if (fromUsb) {
        String q = rxLine;
        q.trim();
        if (q.length()) Serial1.println("PROMPT\t" + q);
      } else {
        handleCommand(rxLine);
      }
      rxLine = "";
    } else if (rxLine.length() < 600) {
      rxLine += c;
    } else {
      rxLine = "";
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(400);
  Serial1.setTX(CYD_TX_PIN);
  Serial1.setRX(CYD_RX_PIN);
  Serial1.begin(BAUD);

  addUser("guest", "Guest");

  Serial.println("=== VORTEX PICO AI COPROCESSOR ===");
  Serial.println("UART1: GP4 TX / GP5 RX");
  Serial.println("USB serial can be used to send a test prompt.");
  Serial1.println("READY\tPICO_AI\tVORTEX");
}

void loop() {
  readPort(Serial1, false);
  readPort(Serial, true);
  delay(2);
}
