/* Vortex AI v3 - ESP32-2432S028 CYD
   Offline hybrid AI: compact trained transformer + SD retrieval + deterministic domain help.
   Purpose command: 208682De
*/
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <FS.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <math.h>
#include <string.h>

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

TFT_eSPI tft=TFT_eSPI();
XPT2046_Touchscreen touch(TOUCH_CS,TOUCH_IRQ);
static const char *MODEL_PATH="/vortex/model.bin";
static const char *TOKENIZER_PATH="/vortex/tokenizer.bin";
static const char *PURPOSE_PATH="/vortex/purpose.txt";
static const char *PURPOSE_CMD="208682De";

String purpose="Help Dzavious Ojelade with tasks, learning, programming, electronics, game development, and general problem solving.";
bool sdOK=false, changingPurpose=false;

struct Config{int vocab_size,dim,hidden_dim,n_layers,n_heads,n_kv_heads,seq_len;};
struct Weights{float *emb,*rms1,*wq,*wk,*wv,*wo,*rms2,*w1,*w2,*w3,*rmsf,*cls;};
struct State{float*x,*xb,*xb2,*hb,*hb2,*q,*k,*v,*att,*logits,*kc,*vc;};
struct Model{Config c{};Weights w{};State s{};uint8_t *blob=nullptr;size_t bytes=0;bool loaded=false;};
Model model;
char **vocab=nullptr;

void *A(size_t n){void*p=ps_malloc(n);return p?p:malloc(n);}
void page(const String&t,const String&b){tft.fillScreen(TFT_BLACK);tft.fillRect(0,0,320,40,TFT_DARKGREY);tft.setTextColor(TFT_WHITE,TFT_DARKGREY);tft.setTextSize(2);tft.setCursor(7,11);tft.print(t);tft.setTextSize(1);tft.setTextColor(TFT_WHITE,TFT_BLACK);int y=55;String line;for(size_t i=0;i<b.length()&&y<232;i++){char c=b[i];if(c=='\n'||line.length()>=48){tft.setCursor(6,y);tft.print(line);y+=12;line="";if(c!='\n')line+=c;}else line+=c;}if(line.length()&&y<232){tft.setCursor(6,y);tft.print(line);}}
void displayInit(){pinMode(TFT_BL,OUTPUT);digitalWrite(TFT_BL,LOW);delay(80);tft.init();tft.setRotation(1);tft.fillScreen(TFT_RED);delay(100);tft.fillScreen(TFT_GREEN);delay(100);tft.fillScreen(TFT_BLUE);delay(100);tft.fillScreen(TFT_BLACK);digitalWrite(TFT_BL,HIGH);}
String readFile(const char*p){if(!sdOK)return"";File f=SD.open(p,FILE_READ);if(!f)return"";String s=f.readString();f.close();return s;}
void savePurpose(){if(!sdOK)return;File f=SD.open(PURPOSE_PATH,FILE_WRITE);if(f){f.print(purpose);f.close();}}
void loadPurpose(){String p=readFile(PURPOSE_PATH);p.trim();if(p.length())purpose=p;}

bool allocState(){int d=model.c.dim,h=model.c.hidden_dim,L=model.c.n_layers,kv=d*model.c.n_kv_heads/model.c.n_heads;
 model.s.x=(float*)A(d*4);model.s.xb=(float*)A(d*4);model.s.xb2=(float*)A(d*4);model.s.hb=(float*)A(h*4);model.s.hb2=(float*)A(h*4);model.s.q=(float*)A(d*4);model.s.k=(float*)A(kv*4);model.s.v=(float*)A(kv*4);model.s.att=(float*)A(model.c.n_heads*model.c.seq_len*4);model.s.logits=(float*)A(model.c.vocab_size*4);model.s.kc=(float*)A((size_t)L*model.c.seq_len*kv*4);model.s.vc=(float*)A((size_t)L*model.c.seq_len*kv*4);
 return model.s.x&&model.s.xb&&model.s.xb2&&model.s.hb&&model.s.hb2&&model.s.q&&model.s.k&&model.s.v&&model.s.att&&model.s.logits&&model.s.kc&&model.s.vc;}

bool loadModel(){if(!sdOK)return false;File f=SD.open(MODEL_PATH,FILE_READ);if(!f)return false;model.bytes=f.size();if(model.bytes<28){f.close();return false;}model.blob=(uint8_t*)A(model.bytes);if(!model.blob){f.close();return false;}if(f.read(model.blob,model.bytes)!=model.bytes){f.close();return false;}f.close();memcpy(&model.c,model.blob,28);if(model.c.vocab_size!=256||model.c.dim<=0||model.c.hidden_dim<=0||model.c.n_layers<=0){return false;}
 uint8_t*p=model.blob+28;float*fp=(float*)p;int d=model.c.dim,h=model.c.hidden_dim,L=model.c.n_layers,head=d/model.c.n_heads,kv=d*model.c.n_kv_heads/model.c.n_heads;model.w.emb=fp;fp+=(size_t)256*d;model.w.rms1=fp;fp+=(size_t)L*d;model.w.wq=fp;fp+=(size_t)L*d*d;model.w.wk=fp;fp+=(size_t)L*d*kv;model.w.wv=fp;fp+=(size_t)L*d*kv;model.w.wo=fp;fp+=(size_t)L*d*d;model.w.rms2=fp;fp+=(size_t)L*d;model.w.w1=fp;fp+=(size_t)L*d*h;model.w.w2=fp+=(size_t)L*h*d;model.w.w3=fp;fp+=(size_t)L*d*h;model.w.rmsf=fp;fp+=d;fp+=(size_t)model.c.seq_len*head/2;fp+=(size_t)model.c.seq_len*head/2;model.w.cls=fp;if(!allocState())return false;memset(model.s.kc,0,(size_t)L*model.c.seq_len*kv*4);memset(model.s.vc,0,(size_t)L*model.c.seq_len*kv*4);model.loaded=true;return true;}

bool loadTokenizer(){if(!sdOK)return false;File f=SD.open(TOKENIZER_PATH,FILE_READ);if(!f)return false;uint32_t mx;if(f.read((uint8_t*)&mx,4)!=4){f.close();return false;}vocab=(char**)A(256*sizeof(char*));if(!vocab){f.close();return false;}for(int i=0;i<256;i++){float score;uint32_t len;if(f.read((uint8_t*)&score,4)!=4||f.read((uint8_t*)&len,4)!=4){f.close();return false;}vocab[i]=(char*)A(len+1);if(!vocab[i]||f.read((uint8_t*)vocab[i],len)!=len){f.close();return false;}vocab[i][len]=0;}f.close();return true;}
float dot(const float*a,const float*b,int n){float s=0;for(int i=0;i<n;i++)s+=a[i]*b[i];return s;}void rms(float*o,const float*x,const float*w,int n){float ss=0;for(int i=0;i<n;i++)ss+=x[i]*x[i];float z=1.0f/sqrtf(ss/n+1e-5f);for(int i=0;i<n;i++)o[i]=x[i]*z*w[i];}void mm(float*o,const float*x,const float*w,int n,int d){for(int i=0;i<d;i++)o[i]=dot(w+(size_t)i*n,x,n);}void sm(float*x,int n){float m=x[0],s=0;for(int i=1;i<n;i++)if(x[i]>m)m=x[i];for(int i=0;i<n;i++){x[i]=expf(x[i]-m);s+=x[i];}for(int i=0;i<n;i++)x[i]/=s;}
float*forward(int tok,int pos){int d=model.c.dim,h=model.c.hidden_dim,L=model.c.n_layers,head=d/model.c.n_heads,kv=d*model.c.n_kv_heads/model.c.n_heads,mul=model.c.n_heads/model.c.n_kv_heads;State&s=model.s;Weights&w=model.w;memcpy(s.x,w.emb+(size_t)tok*d,d*4);for(int l=0;l<L;l++){rms(s.xb,s.x,w.rms1+(size_t)l*d,d);mm(s.q,s.xb,w.wq+(size_t)l*d*d,d,d);mm(s.k,s.xb,w.wk+(size_t)l*d*kv,d,kv);mm(s.v,s.xb,w.wv+(size_t)l*d*kv,d,kv);for(int hh=0;hh<model.c.n_heads;hh++)for(int i=0;i<head;i+=2){float freq=1.0f/powf(10000.0f,(float)i/head),a=pos*freq,c=cosf(a),sn=sinf(a);float*q=s.q+hh*head,q0=q[i],q1=q[i+1];q[i]=q0*c-q1*sn;q[i+1]=q0*sn+q1*c;if(hh<model.c.n_kv_heads){float*k=s.k+hh*head,k0=k[i],k1=k[i+1];k[i]=k0*c-k1*sn;k[i+1]=k0*sn+k1*c;}}size_t off=(size_t)l*model.c.seq_len*kv;memcpy(s.kc+off+pos*kv,s.k,kv*4);memcpy(s.vc+off+pos*kv,s.v,kv*4);for(int hh=0;hh<model.c.n_heads;hh++){float*q=s.q+hh*head,*at=s.att+hh*model.c.seq_len;for(int t=0;t<=pos;t++){float*k=s.kc+off+(size_t)t*kv+(hh/mul)*head;at[t]=dot(q,k,head)/sqrtf(head);}sm(at,pos+1);float*xb=s.xb+hh*head;memset(xb,0,head*4);for(int t=0;t<=pos;t++){float*v=s.vc+off+(size_t)t*kv+(hh/mul)*head;for(int i=0;i<head;i++)xb[i]+=at[t]*v[i];}}mm(s.xb2,s.xb,w.wo+(size_t)l*d*d,d,d);for(int i=0;i<d;i++)s.x[i]+=s.xb2[i];rms(s.xb,s.x,w.rms2+(size_t)l*d,d);mm(s.hb,s.xb,w.w1+(size_t)l*d*h,d,h);mm(s.hb2,s.xb,w.w3+(size_t)l*d*h,d,h);for(int i=0;i<h;i++)s.hb[i]=(s.hb[i]/(1+expf(-s.hb[i])))*s.hb2[i];mm(s.xb2,s.hb,w.w2+(size_t)l*h*d,h,d);for(int i=0;i<d;i++)s.x[i]+=s.xb2[i];}rms(s.x,s.x,w.rmsf,d);mm(s.logits,s.x,w.cls,d,256);return s.logits;}
int enc(const String&p,int*t,int max){int n=0;for(int i=0;i<(int)p.length()&&n<max;i++)t[n++]=(uint8_t)p[i];return n;}

String knowledge(const String&q){if(!sdOK)return"";String best="";int bestScore=0;String nq=q;nq.toLowerCase();String dirs[]={"/vortex/knowledge/physics","/vortex/knowledge/math","/vortex/knowledge/programming","/vortex/knowledge/esp32","/vortex/knowledge/godot","/vortex/knowledge/roblox","/vortex/knowledge/general","/vortex/memory"};for(String dir:dirs){File d=SD.open(dir);if(!d||!d.isDirectory())continue;File f=d.openNextFile();while(f){if(!f.isDirectory()){String n=f.name();if(n.endsWith(".txt")||n.endsWith(".md")){String s=f.readString();String low=s;low.toLowerCase();int score=0;int p=0;while((p=low.indexOf(nq,p))>=0){score+=10;p+=max(1,(int)nq.length());}if(score>bestScore){bestScore=score;best=s.substring(0,min(1000,(int)s.length()));}}}f.close();f=d.openNextFile();}d.close();}return best;}

String domainHelp(String q){String p=q;String l=q;l.toLowerCase();if(l.indexOf("newton")>=0||l.indexOf("physics")>=0||l.indexOf("gravity")>=0||l.indexOf("force")>=0)return"Physics: I can help with force, motion, energy, momentum and gravity. Newton's second law is F = m*a. Near Earth, g is about 9.81 m/s^2.";if(l.indexOf("math")>=0||l.indexOf("equation")>=0||l.indexOf("calculate")>=0||l.indexOf("area")>=0)return"Math: I can work through arithmetic, algebra and geometry step by step. Give me the exact problem and I will break it down.";if(l.indexOf("esp32")>=0||l.indexOf("cyd")>=0||l.indexOf("arduino")>=0)return"ESP32/CYD: your board is the ESP32-2432S028. The common CYD LCD uses SPI with MOSI 13, MISO 12, SCLK 14, CS 15 and DC 2; SD CS is 5 and touch CS is 33.";if(l.indexOf("godot")>=0||l.indexOf("gdscript")>=0)return"Godot: I can help with Godot 4 scenes, nodes, GDScript, C#, 2D/3D systems, UI, networking and debugging.";if(l.indexOf("roblox")>=0||l.indexOf("luau")>=0)return"Roblox: I can help with Luau, RemoteEvents, UI, RPG systems, server/client architecture and Roblox Studio.";if(l.indexOf("program")>=0||l.indexOf("code")>=0||l.indexOf("debug")>=0)return"Programming: I can help write and debug C++, Python, GDScript, C#, JavaScript and Luau. Send the code and the exact error.";if(l=="hello"||l=="hi"||l=="hey")return"Hello Dzavious. I'm Vortex. What are we working on?";if(l.indexOf("who are you")>=0||l.indexOf("what are you")>=0)return"I am Vortex, an offline AI assistant designed to help Dzavious Ojelade.";return"";}

String generate(const String&q){String d=domainHelp(q);String k=knowledge(q);if(d.length()&&k.length())return d+"\n\nSD knowledge:\n"+k;if(d.length())return d;if(k.length())return"I found this on the SD card:\n"+k;if(!model.loaded)return"I don't have a matching SD knowledge file yet. Add .txt or .md files under /vortex/knowledge and I can use them.";String prompt="User: "+q+"\nVortex:";int tok[64],n=enc(prompt,tok,64);if(n<1)return"I could not read that prompt.";memset(model.s.kc,0,(size_t)model.c.n_layers*model.c.seq_len*(model.c.dim*model.c.n_kv_heads/model.c.n_heads)*4);memset(model.s.vc,0,(size_t)model.c.n_layers*model.c.seq_len*(model.c.dim*model.c.n_kv_heads/model.c.n_heads)*4);for(int i=0;i<n-1;i++)forward(tok[i],i);int cur=tok[n-1];String out;for(int i=0;i<48&&n+i<model.c.seq_len;i++){float*lg=forward(cur,n+i);int best=0;for(int j=1;j<256;j++)if(lg[j]>lg[best])best=j;char c=(char)best;if(c<32&&c!='\n'&&c!='\t')break;out+=c;cur=best;if(out.length()>500)break;}return out.length()?out:"I need more information to answer that.";}

void handle(String q){q.trim();if(!q.length())return;if(changingPurpose){purpose=q;savePurpose();changingPurpose=false;Serial.println("VORTEX: My purpose has been updated.");page("PURPOSE","Purpose updated:\n"+purpose);return;}if(q==PURPOSE_CMD){changingPurpose=true;Serial.println("VORTEX: What is my new purpose?");page("PURPOSE","What is my new purpose?\nType the new purpose in Serial.");return;}String l=q;l.toLowerCase();if(l=="show purpose"||l=="what is your purpose"){Serial.println("VORTEX PURPOSE: "+purpose);page("PURPOSE",purpose);return;}Serial.println("Vortex is thinking...");String a=generate(q);Serial.println("VORTEX: "+a);page("VORTEX",a);}

void setup(){Serial.begin(115200);delay(500);displayInit();touch.begin();touch.setRotation(1);sdOK=SD.begin(SD_CS);if(sdOK){if(!SD.exists("/vortex"))SD.mkdir("/vortex");if(!SD.exists("/vortex/knowledge"))SD.mkdir("/vortex/knowledge");loadPurpose();}bool m=loadModel();bool t=m&&loadTokenizer();String s="VORTEX AI v3\nSD: "+String(sdOK?"OK":"MISSING")+"\nModel: "+String(m?"TRAINED":"NOT FOUND")+"\nTokenizer: "+String(t?"OK":"NOT FOUND")+"\nPurpose: "+purpose;page("VORTEX AI v3",s);Serial.println("=== VORTEX AI v3 ===");Serial.println("Purpose: "+purpose);Serial.println("Command 208682De changes purpose.");}
void loop(){if(Serial.available()){String q=Serial.readStringUntil('\n');handle(q);}delay(10);}
