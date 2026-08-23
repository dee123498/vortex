/* VORTEX CYD FRONT-END + WIFI WEB HOST
   ESP32-2432S028 / 2.8in ILI9341 / XPT2046
   Pico UART: CYD GPIO3 RX <- Pico GP4 TX (pin 6)
              CYD GPIO1 TX -> Pico GP5 RX (pin 7)
              CYD GND     <-> Pico GND
*/
#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <SD.h>
#include <FS.h>
#include "vortex_web.h"

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
static constexpr int SCREEN_W = 320, SCREEN_H = 240;
static constexpr int TOUCH_X_MIN = 200, TOUCH_X_MAX = 3900;
static constexpr int TOUCH_Y_MIN = 200, TOUCH_Y_MAX = 3900;

struct User { String id; String name; };
User users[4]; uint8_t userCount = 0; int activeUser = 0;
String inputText, lastReply = "Hello. I'm Vortex.", lastPrompt, picoLine;
bool sdOK=false, wifiOK=false, waiting=false, userPage=false;
bool modelPresent=false, tokenizerPresent=false;
unsigned long lastTouch=0;
const char *keys[] = {"QWERTYUIOP","ASDFGHJKL","ZXCVBNM"};

void setUsersDefault(){users[0]={"guest","Guest"};users[1]={"user1","User 1"};users[2]={"user2","User 2"};users[3]={"user3","User 3"};userCount=4;}
void saveUsers(){if(!sdOK)return;if(!SD.exists("/vortex"))SD.mkdir("/vortex");File f=SD.open("/vortex/users.csv",FILE_WRITE);if(!f)return;for(uint8_t i=0;i<userCount;i++)f.println(users[i].id+","+users[i].name);f.close();}
void loadUsers(){setUsersDefault();if(!sdOK||!SD.exists("/vortex/users.csv")){saveUsers();return;}File f=SD.open("/vortex/users.csv",FILE_READ);if(!f)return;uint8_t n=0;while(f.available()&&n<4){String line=f.readStringUntil('\n');line.trim();int c=line.indexOf(',');if(c<=0)continue;String id=line.substring(0,c),name=line.substring(c+1);id.trim();name.trim();if(id.length())users[n++]={id,name.length()?name:id};}f.close();if(n)userCount=n;}
void loadModelStatus(){modelPresent=sdOK&&SD.exists("/vortex/model.bin");tokenizerPresent=sdOK&&SD.exists("/vortex/tokenizer.bin");}
String wrapText(const String&s,int maxChars){String o;int n=0;for(size_t i=0;i<s.length();i++){char c=s[i];if(c=='\n'){o+='\n';n=0;}else{o+=c;if(++n>=maxChars&&c==' '){o+='\n';n=0;}}}return o;}
void drawHeader(const String&t){tft.fillRect(0,0,SCREEN_W,28,TFT_DARKGREY);tft.setTextSize(2);tft.setTextColor(TFT_WHITE,TFT_DARKGREY);tft.setCursor(6,6);tft.print(t);tft.setTextSize(1);tft.setCursor(236,10);tft.print(wifiOK?"WIFI":"AP");}
void drawChat(){tft.fillScreen(TFT_BLACK);drawHeader("VORTEX");tft.setTextSize(1);tft.setTextColor(TFT_CYAN,TFT_BLACK);tft.setCursor(6,34);tft.print("USER: ");tft.print(users[activeUser].name);tft.setTextColor(TFT_WHITE,TFT_BLACK);String shown=wrapText(lastReply,48);int y=50;String line;for(size_t i=0;i<shown.length()&&y<112;i++){char c=shown[i];if(c=='\n'||line.length()>=48){tft.setCursor(6,y);tft.print(line);y+=11;line="";if(c!='\n')line+=c;}else line+=c;}if(line.length()&&y<112){tft.setCursor(6,y);tft.print(line);}tft.drawRect(4,116,312,22,TFT_DARKGREY);tft.setTextColor(TFT_YELLOW,TFT_BLACK);tft.setCursor(8,123);String in=inputText;if(in.length()>46)in=in.substring(in.length()-46);tft.print(in);if(waiting){tft.setTextColor(TFT_ORANGE,TFT_BLACK);tft.setCursor(245,123);tft.print("thinking");}tft.fillRect(0,141,76,24,TFT_BLUE);tft.fillRect(81,141,76,24,TFT_BLUE);tft.fillRect(162,141,76,24,TFT_BLUE);tft.fillRect(243,141,77,24,TFT_RED);tft.setTextColor(TFT_WHITE,TFT_BLUE);tft.setCursor(17,149);tft.print("USER");tft.setCursor(99,149);tft.print("DEL");tft.setCursor(178,149);tft.print("SPACE");tft.setTextColor(TFT_WHITE,TFT_RED);tft.setCursor(261,149);tft.print("SEND");const int ry[3]={170,188,206};for(int r=0;r<3;r++){int n=strlen(keys[r]),w=SCREEN_W/n;for(int k=0;k<n;k++){int x=k*w;tft.fillRect(x+1,ry[r]+1,w-2,17,TFT_NAVY);tft.setTextColor(TFT_WHITE,TFT_NAVY);tft.setCursor(x+w/2-3,ry[r]+5);tft.print(keys[r][k]);}}}
void drawUserPage(){tft.fillScreen(TFT_BLACK);drawHeader("CHOOSE USER");tft.setTextSize(1);tft.setTextColor(TFT_LIGHTGREY,TFT_BLACK);tft.setCursor(8,35);tft.print("Select a profile. Memory stays per profile.");for(int i=0;i<4;i++){int x=(i%2)*160,y=58+(i/2)*65;uint16_t bg=i==activeUser?TFT_DARKGREEN:TFT_BLUE;tft.fillRoundRect(x+8,y,144,54,6,bg);tft.setTextColor(TFT_WHITE,bg);tft.setTextSize(2);tft.setCursor(x+20,y+18);tft.print(users[i].name);}tft.setTextSize(1);tft.setCursor(10,205);tft.setTextColor(TFT_CYAN,TFT_BLACK);tft.print("Profiles are selected, not biometric recognition.");}
void audioAck(){pinMode(AUDIO_PIN,OUTPUT);tone(AUDIO_PIN,880,45);delay(55);noTone(AUDIO_PIN);}
void sendUserToPico(){PicoSerial.println("USER\t"+users[activeUser].id+"\t"+users[activeUser].name);}
void sendPrompt(){if(waiting||!inputText.length())return;lastPrompt=inputText;lastReply="Vortex is thinking...";waiting=true;drawChat();sendUserToPico();PicoSerial.println("PROMPT\t"+inputText);inputText="";}
void handlePicoLine(String line){line.trim();if(!line.length())return;if(line=="BEGIN"){waiting=true;return;}if(line=="END"){waiting=false;audioAck();drawChat();vortexWebOnPicoLine(line);return;}if(line.startsWith("TEXT\t")){lastReply=line.substring(5);lastReply.replace("\\n","\n");vortexWebOnPicoLine(line);return;}if(line.startsWith("USER_OK\t")||line.startsWith("READY\t")||line.startsWith("PONG\t")){vortexWebOnPicoLine(line);return;}if(line.startsWith("ERROR\t")){waiting=false;lastReply="Pico error: "+line.substring(6);drawChat();}vortexWebOnPicoLine(line);}
void readPico(){while(PicoSerial.available()){char c=(char)PicoSerial.read();if(c=='\r')continue;if(c=='\n'){handlePicoLine(picoLine);picoLine="";}else if(picoLine.length()<700)picoLine+=c;else picoLine="";}}
void touchPoint(int&x,int&y){TS_Point p=touch.getPoint();x=map(p.y,TOUCH_Y_MIN,TOUCH_Y_MAX,0,SCREEN_W);y=map(p.x,TOUCH_X_MIN,TOUCH_X_MAX,SCREEN_H,0);x=constrain(x,0,SCREEN_W-1);y=constrain(y,0,SCREEN_H-1);}
void keyPress(int x,int y){if(userPage){if(y>=58&&y<188){int col=x/160,row=(y-58)/65,idx=row*2+col;if(idx>=0&&idx<4){activeUser=idx;sendUserToPico();userPage=false;inputText="";drawChat();}}return;}if(y>=206){int n=strlen(keys[2]),idx=x/(SCREEN_W/n);if(idx>=0&&idx<n)inputText+=keys[2][idx];drawChat();return;}if(y>=188){int n=strlen(keys[1]),idx=x/(SCREEN_W/n);if(idx>=0&&idx<n)inputText+=keys[1][idx];drawChat();return;}if(y>=170){int n=strlen(keys[0]),idx=x/(SCREEN_W/n);if(idx>=0&&idx<n)inputText+=keys[0][idx];drawChat();return;}if(y>=141&&y<166){if(x<76){userPage=true;drawUserPage();}else if(x<157){if(inputText.length())inputText.remove(inputText.length()-1);drawChat();}else if(x<239){inputText+=' ';drawChat();}else sendPrompt();}}

void setup(){Serial.begin(115200);delay(300);pinMode(TFT_BL,OUTPUT);digitalWrite(TFT_BL,LOW);tft.init();tft.setRotation(1);tft.fillScreen(TFT_BLACK);touch.begin();touch.setRotation(1);digitalWrite(TFT_BL,HIGH);PicoSerial.begin(BAUD,SERIAL_8N1,3,1);sdOK=SD.begin(SD_CS);if(sdOK&&!SD.exists("/vortex"))SD.mkdir("/vortex");loadUsers();loadModelStatus();drawChat();PicoSerial.println("HELLO");delay(100);sendUserToPico();vortexWebBegin();Serial.printf("VORTEX CYD SD=%s model=%s tokenizer=%s\n",sdOK?"OK":"MISSING",modelPresent?"FOUND":"MISSING",tokenizerPresent?"FOUND":"MISSING");}
void loop(){readPico();vortexWebLoop();if(touch.touched()&&millis()-lastTouch>180){lastTouch=millis();int x,y;touchPoint(x,y);keyPress(x,y);}delay(4);}
