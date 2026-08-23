/*
  VORTEX CYD FRONT-END - ESP32-2432S028
  Target: 2.8" ILI9341 CYD with XPT2046 touch.

  Responsibilities:
    - TFT UI + touch keyboard
    - user selection / per-user identity
    - SD-card storage
    - Wi-Fi status
    - UART bridge to Pico AI
    - optional speaker acknowledgement on GPIO26

  Pico link on the CYD P1 connector:
    P1 VIN - TX(GPIO1) - RX(GPIO3) - GND
    Pico GP4 TX -> CYD RX(GPIO3)
    Pico GP5 RX <- CYD TX(GPIO1)
    Pico GND   -> CYD GND

  GPIO1/GPIO3 are also the USB-UART pins. Disconnect the Pico from P1 while
  flashing/debugging the CYD over USB if serial traffic interferes.
*/

#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <SD.h>
#include <FS.h>

#define USER_SETUP_LOADED
#define ILI9341_DRIVER
#define TFT_WIDTH 240
#define TFT_HEIGHT 320
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS 15
#define TFT_DC 2
#define TFT_RST -1
#define LOAD_GLCD
#define SPI_FREQUENCY 40000000
#define SPI_READ_FREQUENCY 20000000

#define SD_CS 5
#define TOUCH_CS 33
#define TOUCH_IRQ 36
#define TFT_BL 21
#define AUDIO_PIN 26

#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

TFT_eSPI tft;
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);
HardwareSerial PicoSerial(2);

static constexpr uint32_t BAUD = 115200;
static constexpr int SCREEN_W = 320;
static constexpr int SCREEN_H = 240;

// Common XPT2046 calibration range. Keep these in one place so the panel can
// be corrected without changing the UI code.
static constexpr int TOUCH_X_MIN = 200;
static constexpr int TOUCH_X_MAX = 3900;
static constexpr int TOUCH_Y_MIN = 200;
static constexpr int TOUCH_Y_MAX = 3900;

struct User { String id; String name; };
User users[4];
uint8_t userCount = 0;
int activeUser = 0;

String inputText;
String lastReply = "Hello. I'm Vortex.";
String lastPrompt;
String picoLine;
bool sdOK = false;
bool wifiOK = false;
bool waiting = false;
bool userPage = false;
bool modelPresent = false;
bool tokenizerPresent = false;
unsigned long lastTouch = 0;

const char *keys[] = { "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM" };

void setUsersDefault() {
  users[0] = {"guest", "Guest"};
  users[1] = {"user1", "User 1"};
  users[2] = {"user2", "User 2"};
  users[3] = {"user3", "User 3"};
  userCount = 4;
}

void saveUsers() {
  if (!sdOK) return;
  if (!SD.exists("/vortex")) SD.mkdir("/vortex");
  File f = SD.open("/vortex/users.csv", FILE_WRITE);
  if (!f) return;
  for (uint8_t i = 0; i < userCount; ++i) f.println(users[i].id + "," + users[i].name);
  f.close();
}

void loadUsers() {
  setUsersDefault();
  if (!sdOK || !SD.exists("/vortex/users.csv")) { saveUsers(); return; }
  File f = SD.open("/vortex/users.csv", FILE_READ);
  if (!f) return;
  uint8_t n = 0;
  while (f.available() && n < 4) {
    String line = f.readStringUntil('\n');
    line.trim();
    int comma = line.indexOf(',');
    if (comma <= 0) continue;
    String id = line.substring(0, comma);
    String name = line.substring(comma + 1);
    id.trim(); name.trim();
    if (!id.length()) continue;
    users[n++] = {id, name.length() ? name : id};
  }
  f.close();
  if (n) userCount = n;
}

void loadModelStatus() {
  modelPresent = sdOK && SD.exists("/vortex/model.bin");
  tokenizerPresent = sdOK && SD.exists("/vortex/tokenizer.bin");
}

void drawHeader(const String &title) {
  tft.fillRect(0, 0, SCREEN_W, 28, TFT_DARKGREY);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.setCursor(6, 6);
  tft.print(title);
  tft.setTextSize(1);
  tft.setCursor(236, 10);
  tft.print(wifiOK ? "WIFI" : "OFF");
}

String wrapText(const String &s, int maxChars) {
  String out;
  int count = 0;
  for (size_t i = 0; i < s.length(); ++i) {
    char c = s[i];
    if (c == '\n') { out += '\n'; count = 0; }
    else {
      out += c;
      if (++count >= maxChars && c == ' ') { out += '\n'; count = 0; }
    }
  }
  return out;
}

void drawChat() {
  tft.fillScreen(TFT_BLACK);
  drawHeader("VORTEX");

  tft.setTextSize(1);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(6, 34);
  tft.print("USER: ");
  tft.print(users[activeUser].name);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  String shown = wrapText(lastReply, 48);
  int y = 50;
  String line;
  for (size_t i = 0; i < shown.length() && y < 112; ++i) {
    char c = shown[i];
    if (c == '\n' || line.length() >= 48) {
      tft.setCursor(6, y); tft.print(line); y += 11; line = "";
      if (c != '\n') line += c;
    } else line += c;
  }
  if (line.length() && y < 112) { tft.setCursor(6, y); tft.print(line); }

  tft.drawRect(4, 116, 312, 22, TFT_DARKGREY);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(8, 123);
  String shownInput = inputText;
  if (shownInput.length() > 46) shownInput = shownInput.substring(shownInput.length() - 46);
  tft.print(shownInput);
  if (waiting) {
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    tft.setCursor(245, 123); tft.print("thinking");
  }

  tft.fillRect(0, 141, 76, 24, TFT_BLUE);
  tft.fillRect(81, 141, 76, 24, TFT_BLUE);
  tft.fillRect(162, 141, 76, 24, TFT_BLUE);
  tft.fillRect(243, 141, 77, 24, TFT_RED);
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.setCursor(17, 149); tft.print("USER");
  tft.setCursor(99, 149); tft.print("DEL");
  tft.setCursor(178, 149); tft.print("SPACE");
  tft.setTextColor(TFT_WHITE, TFT_RED);
  tft.setCursor(261, 149); tft.print("SEND");

  const int rowY[3] = {170, 188, 206};
  for (int r = 0; r < 3; ++r) {
    int count = strlen(keys[r]);
    int keyW = SCREEN_W / count;
    for (int k = 0; k < count; ++k) {
      int x = k * keyW;
      tft.fillRect(x + 1, rowY[r] + 1, keyW - 2, 17, TFT_NAVY);
      tft.setTextColor(TFT_WHITE, TFT_NAVY);
      tft.setCursor(x + keyW / 2 - 3, rowY[r] + 5);
      tft.print(keys[r][k]);
    }
  }
}

void drawUserPage() {
  tft.fillScreen(TFT_BLACK);
  drawHeader("CHOOSE USER");
  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setCursor(8, 35);
  tft.print("Select a profile. Vortex keeps memory per profile.");

  for (int i = 0; i < 4; ++i) {
    int x = (i % 2) * 160;
    int y = 58 + (i / 2) * 65;
    uint16_t bg = i == activeUser ? TFT_DARKGREEN : TFT_BLUE;
    tft.fillRoundRect(x + 8, y, 144, 54, 6, bg);
    tft.setTextColor(TFT_WHITE, bg);
    tft.setTextSize(2);
    tft.setCursor(x + 20, y + 18);
    tft.print(users[i].name);
  }

  tft.setTextSize(1);
  tft.setCursor(10, 205);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.print("Profiles identify users by selection, not biometrics.");
}

void audioAck() {
  pinMode(AUDIO_PIN, OUTPUT);
  tone(AUDIO_PIN, 880, 45);
  delay(55);
  tone(AUDIO_PIN, 0, 10);
}

void sendUserToPico() {
  PicoSerial.println("USER\t" + users[activeUser].id + "\t" + users[activeUser].name);
}

void sendPrompt() {
  if (waiting || !inputText.length()) return;
  lastPrompt = inputText;
  lastReply = "Vortex is thinking...";
  waiting = true;
  drawChat();
  sendUserToPico();
  PicoSerial.println("PROMPT\t" + inputText);
  inputText = "";
}

void handlePicoLine(String line) {
  line.trim();
  if (!line.length()) return;
  if (line == "BEGIN") { waiting = true; return; }
  if (line == "END") { waiting = false; audioAck(); drawChat(); return; }
  if (line.startsWith("TEXT\t")) {
    lastReply = line.substring(5);
    lastReply.replace("\\n", "\n");
    return;
  }
  if (line.startsWith("USER_OK\t") || line.startsWith("READY\t") || line.startsWith("PONG\t")) {
    waiting = false;
    return;
  }
  if (line.startsWith("ERROR\t")) {
    waiting = false;
    lastReply = "Pico error: " + line.substring(6);
    drawChat();
  }
}

void readPico() {
  while (PicoSerial.available()) {
    char c = (char)PicoSerial.read();
    if (c == '\r') continue;
    if (c == '\n') { handlePicoLine(picoLine); picoLine = ""; }
    else if (picoLine.length() < 700) picoLine += c;
    else picoLine = "";
  }
}

bool wifiConnectFromSD() {
  if (!sdOK || !SD.exists("/vortex/wifi.txt")) return false;
  File f = SD.open("/vortex/wifi.txt", FILE_READ);
  if (!f) return false;
  String ssid = f.readStringUntil('\n');
  String pass = f.readString();
  f.close();
  ssid.trim(); pass.trim();
  if (!ssid.length()) return false;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) delay(100);
  return WiFi.status() == WL_CONNECTED;
}

void touchPoint(int &x, int &y) {
  TS_Point p = touch.getPoint();
  // Rotation 1 mapping used by the common 240x320 XPT2046 panel.
  x = map(p.y, TOUCH_Y_MIN, TOUCH_Y_MAX, 0, SCREEN_W);
  y = map(p.x, TOUCH_X_MIN, TOUCH_X_MAX, SCREEN_H, 0);
  x = constrain(x, 0, SCREEN_W - 1);
  y = constrain(y, 0, SCREEN_H - 1);
}

void keyPress(int x, int y) {
  if (userPage) {
    if (y >= 58 && y < 188) {
      int col = x / 160;
      int row = (y - 58) / 65;
      int idx = row * 2 + col;
      if (idx >= 0 && idx < 4) {
        activeUser = idx;
        sendUserToPico();
        userPage = false;
        inputText = "";
        drawChat();
      }
    }
    return;
  }

  if (y >= 206 && y < 240) {
    int count = strlen(keys[2]);
    int idx = x / (SCREEN_W / count);
    if (idx >= 0 && idx < count) inputText += keys[2][idx];
    drawChat(); return;
  }
  if (y >= 188 && y < 206) {
    int count = strlen(keys[1]);
    int idx = x / (SCREEN_W / count);
    if (idx >= 0 && idx < count) inputText += keys[1][idx];
    drawChat(); return;
  }
  if (y >= 170 && y < 188) {
    int count = strlen(keys[0]);
    int idx = x / (SCREEN_W / count);
    if (idx >= 0 && idx < count) inputText += keys[0][idx];
    drawChat(); return;
  }

  if (y >= 141 && y < 166) {
    if (x < 76) {
      userPage = true; drawUserPage();
    } else if (x < 157) {
      if (inputText.length()) inputText.remove(inputText.length() - 1);
      drawChat();
    } else if (x < 239) {
      inputText += ' '; drawChat();
    } else sendPrompt();
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, LOW);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  touch.begin();
  touch.setRotation(1);
  digitalWrite(TFT_BL, HIGH);

  // UART2 is routed to the CYD P1 pins: RX=GPIO3, TX=GPIO1.
  PicoSerial.begin(BAUD, SERIAL_8N1, 3, 1);

  sdOK = SD.begin(SD_CS);
  if (sdOK && !SD.exists("/vortex")) SD.mkdir("/vortex");
  loadUsers();
  loadModelStatus();
  wifiOK = wifiConnectFromSD();

  drawChat();
  PicoSerial.println("HELLO");
  delay(100);
  sendUserToPico();

  Serial.println("=== VORTEX CYD FRONT-END ===");
  Serial.printf("SD=%s model=%s tokenizer=%s WiFi=%s\n",
                sdOK ? "OK" : "MISSING",
                modelPresent ? "FOUND" : "MISSING",
                tokenizerPresent ? "FOUND" : "MISSING",
                wifiOK ? "OK" : "OFFLINE");
}

void loop() {
  readPico();

  if (touch.touched() && millis() - lastTouch > 180) {
    lastTouch = millis();
    int x, y;
    touchPoint(x, y);
    keyPress(x, y);
  }

  delay(4);
}
