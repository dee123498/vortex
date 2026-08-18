/* Vortex 2.0 fixed CYD firmware */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <SD.h>
#include <FS.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ------------------------- CYD HARDWARE -------------------------
// ESP32-2432S028 / "Cheap Yellow Display" (2.8", 320x240).
// These definitions are deliberately in the sketch so TFT_eSPI does not
// depend on a user's global User_Setup.h. They must appear before TFT_eSPI.
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

#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

TFT_eSPI tft = TFT_eSPI();
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

// ------------------------- VORTEX CONFIG -------------------------
static const char *MODEL_PATH = "/vortex/model.bin";
static const char *TOKENIZER_PATH = "/vortex/tokenizer.bin";
static const int DEFAULT_STEPS = 96;

String wifiSSID;
String wifiPassword;
String locationName = "My Location";
float latitude = 41.1170f;
float longitude = -88.2217f;
bool fahrenheit = true;
bool wifiOK = false;
bool sdOK = false;

// ------------------------- MODEL TYPES -------------------------
typedef struct { int vocab_size; int dim; int hidden_dim; int n_layers; int n_heads; int n_kv_heads; int seq_len; } Config;
typedef struct { float prob; int index; } ProbIndex;
typedef struct {
  char **vocab; float *vocab_scores; int vocab_size; unsigned int max_token_length; unsigned char byte_pieces[512];
} Tokenizer;
typedef struct {
  float *token_embedding_table,*rms_att_weight,*wq,*wk,*wv,*wo,*rms_ffn_weight,*w1,*w2,*w3,*rms_final_weight,*wcls;
} TransformerWeights;
typedef struct {
  float *x,*xb,*xb2,*hb,*hb2,*q,*k,*v,*att,*logits,*key_cache,*value_cache;
} RunState;
struct VortexTransformer { Config config{}; TransformerWeights weights{}; RunState state{}; uint8_t *checkpoint=nullptr; size_t checkpointSize=0; bool loaded=false; };
VortexTransformer model;
Tokenizer tokenizer{};

// ------------------------- UI -------------------------
void header(const String &title) {
  tft.fillScreen(TFT_BLACK);
  tft.fillRect(0,0,320,42,TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE,TFT_DARKGREY); tft.setTextSize(2); tft.setCursor(8,12); tft.print(title);
  tft.setTextSize(1); tft.setCursor(240,15); tft.print(wifiOK ? "ONLINE" : "OFFLINE");
}
void textPage(const String &title,const String &body) {
  header(title); tft.setTextSize(1); tft.setTextColor(TFT_WHITE,TFT_BLACK); int y=55; String line;
  for(size_t i=0;i<body.length() && y<230;++i){ char c=body[i]; if(c=='\n'||line.length()>=48){tft.setCursor(6,y);tft.print(line);y+=12;line="";if(c!='\n')line+=c;}else line+=c; }
  if(line.length()&&y<230){tft.setCursor(6,y);tft.print(line);}
}

// ------------------------- DISPLAY FIX -------------------------
bool initDisplay() {
  // Backlight OFF while the controller is reset/initialized. This prevents
  // a bright white panel from looking like a failed boot.
  pinMode(TFT_BL,OUTPUT); digitalWrite(TFT_BL,LOW); delay(80);
  tft.init();
  tft.setRotation(1);
  tft.setSwapBytes(true);
  tft.fillScreen(TFT_BLACK);
  delay(50);
  // Hardware-visible SPI/controller test.
  tft.fillScreen(TFT_RED); delay(120);
  tft.fillScreen(TFT_GREEN); delay(120);
  tft.fillScreen(TFT_BLUE); delay(120);
  tft.fillScreen(TFT_BLACK);
  digitalWrite(TFT_BL,HIGH);
  return true;
}

// ------------------------- SD CONFIG -------------------------
String sdText(const char *path){if(!sdOK)return "";File f=SD.open(path,FILE_READ);if(!f)return "";String s=f.readString();f.close();return s;}
void loadConfig(){
  if(!sdOK)return; if(!SD.exists("/vortex"))SD.mkdir("/vortex");
  String w=sdText("/vortex/wifi.txt"); int p=w.indexOf('\n');
  if(p>=0){wifiSSID=w.substring(0,p);wifiSSID.trim();wifiPassword=w.substring(p+1);wifiPassword.trim();}
  String c=sdText("/vortex/config.txt"); int i=c.indexOf("latitude=");if(i>=0)latitude=c.substring(i+9).toFloat();
  i=c.indexOf("longitude=");if(i>=0)longitude=c.substring(i+10).toFloat();
  i=c.indexOf("location=");if(i>=0){locationName=c.substring(i+9);locationName.trim();}
  i=c.indexOf("fahrenheit=");if(i>=0)fahrenheit=c.substring(i+11).toInt()!=0;
}

void *psramAlloc(size_t n){void *p=ps_malloc(n);if(!p)p=malloc(n);return p;}
bool allocRunState(){
  Config *p=&model.config; int kv_dim=(p->dim*p->n_kv_heads)/p->n_heads;
  model.state.x=(float*)psramAlloc(p->dim*sizeof(float)); model.state.xb=(float*)psramAlloc(p->dim*sizeof(float)); model.state.xb2=(float*)psramAlloc(p->dim*sizeof(float));
  model.state.hb=(float*)psramAlloc(p->hidden_dim*sizeof(float)); model.state.hb2=(float*)psramAlloc(p->hidden_dim*sizeof(float)); model.state.q=(float*)psramAlloc(p->dim*sizeof(float));
  model.state.k=(float*)psramAlloc(kv_dim*sizeof(float)); model.state.v=(float*)psramAlloc(kv_dim*sizeof(float)); model.state.att=(float*)psramAlloc((size_t)p->n_heads*p->seq_len*sizeof(float));
  model.state.logits=(float*)psramAlloc(p->vocab_size*sizeof(float)); model.state.key_cache=(float*)psramAlloc((size_t)p->n_layers*p->seq_len*kv_dim*sizeof(float)); model.state.value_cache=(float*)psramAlloc((size_t)p->n_layers*p->seq_len*kv_dim*sizeof(float));
  return model.state.x&&model.state.xb&&model.state.xb2&&model.state.hb&&model.state.hb2&&model.state.q&&model.state.k&&model.state.v&&model.state.att&&model.state.logits&&model.state.key_cache&&model.state.value_cache;
}
void freeModel(){if(model.checkpoint){free(model.checkpoint);model.checkpoint=nullptr;}model.loaded=false;}

bool loadCheckpoint(){
  if(!sdOK)return false; File f=SD.open(MODEL_PATH,FILE_READ);if(!f)return false;model.checkpointSize=f.size();
  if(model.checkpointSize<sizeof(Config)){f.close();return false;} model.checkpoint=(uint8_t*)psramAlloc(model.checkpointSize);if(!model.checkpoint){f.close();return false;}
  size_t got=f.read(model.checkpoint,model.checkpointSize);f.close();if(got!=model.checkpointSize){freeModel();return false;}
  memcpy(&model.config,model.checkpoint,sizeof(Config)); if(model.config.n_heads<=0||model.config.dim<=0||model.config.n_layers<=0||model.config.vocab_size<=0){freeModel();return false;}
  uint8_t *ptr=model.checkpoint+sizeof(Config); float *fp=(float*)ptr; int head=model.config.dim/model.config.n_heads; unsigned long long L=model.config.n_layers;
  model.weights.token_embedding_table=fp;fp+=(unsigned long long)model.config.vocab_size*model.config.dim;
  model.weights.rms_att_weight=fp;fp+=L*model.config.dim; model.weights.wq=fp;fp+=L*model.config.dim*model.config.n_heads*head;
  model.weights.wk=fp;fp+=L*model.config.dim*model.config.n_kv_heads*head; model.weights.wv=fp;fp+=L*model.config.dim*model.config.n_kv_heads*head;
  model.weights.wo=fp;fp+=L*model.config.n_heads*head*model.config.dim; model.weights.rms_ffn_weight=fp;fp+=L*model.config.dim;
  model.weights.w1=fp;fp+=L*model.config.dim*model.config.hidden_dim; model.weights.w2=fp;fp+=L*model.config.hidden_dim*model.config.dim;
  model.weights.w3=fp;fp+=L*model.config.dim*model.config.hidden_dim; model.weights.rms_final_weight=fp;fp+=model.config.dim;
  fp+=model.config.seq_len*head/2;fp+=model.config.seq_len*head/2;model.weights.wcls=fp;
  if(!allocRunState()){freeModel();return false;}
  size_t cache=(size_t)model.config.n_layers*model.config.seq_len*(model.config.dim*model.config.n_kv_heads/model.config.n_heads)*sizeof(float);
  memset(model.state.key_cache,0,cache);memset(model.state.value_cache,0,cache);model.loaded=true;return true;
}

bool loadTokenizer(){
  if(!sdOK)return false;File f=SD.open(TOKENIZER_PATH,FILE_READ);if(!f)return false;uint32_t maxLen=0;if(f.read((uint8_t*)&maxLen,4)!=4){f.close();return false;}
  tokenizer.vocab_size=model.config.vocab_size;tokenizer.max_token_length=maxLen;tokenizer.vocab=(char**)psramAlloc((size_t)tokenizer.vocab_size*sizeof(char*));tokenizer.vocab_scores=(float*)psramAlloc((size_t)tokenizer.vocab_size*sizeof(float));
  if(!tokenizer.vocab||!tokenizer.vocab_scores){f.close();return false;}
  for(int i=0;i<tokenizer.vocab_size;i++){float score=0;uint32_t len=0;if(f.read((uint8_t*)&score,4)!=4||f.read((uint8_t*)&len,4)!=4){f.close();return false;}tokenizer.vocab_scores[i]=score;tokenizer.vocab[i]=(char*)psramAlloc(len+1);if(!tokenizer.vocab[i]){f.close();return false;}if(f.read((uint8_t*)tokenizer.vocab[i],len)!=len){f.close();return false;}tokenizer.vocab[i][len]=0;}
  f.close();for(int i=0;i<256;i++)tokenizer.byte_pieces[i]=(unsigned char)i;return true;
}

float dot(const float*a,const float*b,int n){float s=0;for(int i=0;i<n;i++)s+=a[i]*b[i];return s;}
void rmsnorm(float*o,const float*x,const float*w,int n){float ss=0;for(int i=0;i<n;i++)ss+=x[i]*x[i];ss=1.0f/sqrtf(ss/n+1e-5f);for(int i=0;i<n;i++)o[i]=w[i]*x[i]*ss;}
void softmax(float*x,int n){float mx=x[0];for(int i=1;i<n;i++)if(x[i]>mx)mx=x[i];float sum=0;for(int i=0;i<n;i++){x[i]=expf(x[i]-mx);sum+=x[i];}for(int i=0;i<n;i++)x[i]/=sum;}
void matmul(float*out,const float*x,const float*w,int n,int d){for(int i=0;i<d;i++)out[i]=dot(w+(size_t)i*n,x,n);}
float silu(float x){return x/(1.0f+expf(-x));}

float*forward(int token,int pos){
  Config*p=&model.config;TransformerWeights*w=&model.weights;RunState*s=&model.state;int dim=p->dim,hidden=p->hidden_dim,layers=p->n_layers;int head=dim/p->n_heads;int kvdim=(dim*p->n_kv_heads)/p->n_heads;int kvmul=p->n_heads/p->n_kv_heads;
  memcpy(s->x,w->token_embedding_table+(size_t)token*dim,dim*sizeof(float));
  for(int l=0;l<layers;l++){rmsnorm(s->xb,s->x,w->rms_att_weight+(size_t)l*dim,dim);matmul(s->q,s->xb,w->wq+(size_t)l*dim*dim,dim,dim);matmul(s->k,s->xb,w->wk+(size_t)l*dim*kvdim,dim,kvdim);matmul(s->v,s->xb,w->wv+(size_t)l*dim*kvdim,dim,kvdim);
    for(int h=0;h<p->n_heads;h++)for(int i=0;i<head;i+=2){float freq=1.0f/powf(10000.0f,(float)i/head);float val=pos*freq,c=cosf(val),sn=sinf(val);float*q=s->q+h*head,q0=q[i],q1=q[i+1];q[i]=q0*c-q1*sn;q[i+1]=q0*sn+q1*c;if(h<p->n_kv_heads){float*k=s->k+h*head,k0=k[i],k1=k[i+1];k[i]=k0*c-k1*sn;k[i+1]=k0*sn+k1*c;}}
    size_t layerOff=(size_t)l*p->seq_len*kvdim;memcpy(s->key_cache+layerOff+pos*kvdim,s->k,kvdim*sizeof(float));memcpy(s->value_cache+layerOff+pos*kvdim,s->v,kvdim*sizeof(float));
    for(int h=0;h<p->n_heads;h++){float*q=s->q+h*head,*att=s->att+h*p->seq_len;for(int t=0;t<=pos;t++){float*kk=s->key_cache+layerOff+(size_t)t*kvdim+(h/kvmul)*head;att[t]=dot(q,kk,head)/sqrtf((float)head);}softmax(att,pos+1);float*xb=s->xb+h*head;memset(xb,0,head*sizeof(float));for(int t=0;t<=pos;t++){float*vv=s->value_cache+layerOff+(size_t)t*kvdim+(h/kvmul)*head;for(int i=0;i<head;i++)xb[i]+=att[t]*vv[i];}}
    matmul(s->xb2,s->xb,w->wo+(size_t)l*dim*dim,dim,dim);for(int i=0;i<dim;i++)s->x[i]+=s->xb2[i];rmsnorm(s->xb,s->x,w->rms_ffn_weight+(size_t)l*dim,dim);matmul(s->hb,s->xb,w->w1+(size_t)l*hidden*dim,dim,hidden);matmul(s->hb2,s->xb,w->w3+(size_t)l*hidden*dim,dim,hidden);for(int i=0;i<hidden;i++)s->hb[i]=silu(s->hb[i])*s->hb2[i];matmul(s->xb2,s->hb,w->w2+(size_t)l*dim*hidden,hidden,dim);for(int i=0;i<dim;i++)s->x[i]+=s->xb2[i];}
  rmsnorm(s->x,s->x,w->rms_final_weight,dim);matmul(s->logits,s->x,w->wcls,dim,p->vocab_size);return s->logits;
}

uint32_t rngState=0xA53C9E11;uint32_t xrng(){rngState^=rngState<<13;rngState^=rngState>>17;rngState^=rngState<<5;return rngState;}
int sampleToken(float*logits,int n,float temperature=.75f,float topp=.90f){if(temperature<=0){int bi=0;for(int i=1;i<n;i++)if(logits[i]>logits[bi])bi=i;return bi;}for(int i=0;i<n;i++)logits[i]/=temperature;softmax(logits,n);const int K=64;int ids[K];float ps[K];int count=0;for(int i=0;i<n;i++){float p=logits[i];if(p<=0)continue;int j=count<K?count:K-1;if(count<K)count++;else if(p<=ps[K-1])continue;while(j>0&&ps[j-1]<p){if(j<K){ps[j]=ps[j-1];ids[j]=ids[j-1];}j--;}ps[j]=p;ids[j]=i;}float total=0;for(int i=0;i<count;i++)total+=ps[i];float cutoff=total*topp,r=(xrng()/(float)UINT32_MAX)*cutoff,acc=0;for(int i=0;i<count;i++){acc+=ps[i];if(acc>=r)return ids[i];}return ids[0];}

int encodePrompt(const String&prompt,int*tokens,int maxTokens){if(!tokenizer.vocab)return 0;int n=0;for(size_t pos=0;pos<prompt.length()&&n<maxTokens;){int best=-1,bestLen=0;for(int i=0;i<tokenizer.vocab_size;i++){const char*v=tokenizer.vocab[i];int len=strlen(v);if(len>bestLen&&pos+len<=prompt.length()&&strncmp(prompt.c_str()+pos,v,len)==0){best=i;bestLen=len;}}if(best>=0){tokens[n++]=best;pos+=bestLen;}else{tokens[n++]=(unsigned char)prompt[pos++];}}return n;}
String knowledgeContext(const String&q){if(!sdOK||!SD.exists("/vortex/knowledge"))return "";File d=SD.open("/vortex/knowledge");if(!d||!d.isDirectory())return "";String best="";int scoreBest=0;File f=d.openNextFile();String nq=q;nq.toLowerCase();while(f){if(!f.isDirectory()){String name=f.name();if(name.endsWith(".txt")||name.endsWith(".md")){String ss=f.readString();String low=ss;low.toLowerCase();int score=0,p=0;while((p=low.indexOf(nq,p))>=0){score+=10;p+=max(1,(int)nq.length());}if(score>scoreBest){scoreBest=score;best=ss.substring(0,min((int)ss.length(),700));}}}f.close();f=d.openNextFile();}d.close();return best;}
String runVortex(const String&userPrompt){if(!model.loaded)return "Vortex neural model is not loaded. Put model.bin and tokenizer.bin in /vortex on the SD card.";int tokens[256];String context=knowledgeContext(userPrompt);String prompt="You are Vortex, a helpful offline AI assistant.\nUser: "+userPrompt+"\n";if(context.length())prompt+="Relevant memory:\n"+context+"\n";prompt+="Vortex:";int n=encodePrompt(prompt,tokens,256);if(n<=0)return "Tokenizer could not encode the prompt.";int maxPos=min(n,model.config.seq_len-1);String out;unsigned long started=millis();for(int i=0;i<maxPos;i++)forward(tokens[i],i);int tok=tokens[maxPos-1];for(int step=0;step<DEFAULT_STEPS&&maxPos+step<model.config.seq_len;step++){float*logits=forward(tok,maxPos+step);int next=sampleToken(logits,model.config.vocab_size,.72f,.90f);if(next<0||next>=tokenizer.vocab_size)break;const char*piece=tokenizer.vocab[next];if(piece)out+=piece;tok=next;if(piece&&(strstr(piece,"</s>")||strstr(piece,"<eos>")))break;if(out.length()>700)break;}Serial.printf("Vortex generated %d chars in %.2fs\n",out.length(),(millis()-started)/1000.0f);return out.length()?out:"(Vortex produced no text.)";}

String weather(){if(!wifiOK)return "Offline. Weather requires Wi-Fi.";HTTPClient h;String unit=fahrenheit?"fahrenheit":"celsius";String url="https://api.open-meteo.com/v1/forecast?latitude="+String(latitude,5)+"&longitude="+String(longitude,5)+"&current=temperature_2m,apparent_temperature,wind_speed_10m,weather_code&temperature_unit="+unit+"&wind_speed_unit=mph";h.begin(url);int code=h.GET();if(code!=200){h.end();return "Weather request failed.";}DynamicJsonDocument doc(8192);auto err=deserializeJson(doc,h.getString());h.end();if(err)return "Weather JSON failed.";JsonObject c=doc["current"];float t=c["temperature_2m"]|0.0f,feels=c["apparent_temperature"]|0.0f,wind=c["wind_speed_10m"]|0.0f;int wc=c["weather_code"]|-1;return locationName+"\nTemp: "+String(t,1)+(fahrenheit?" F":" C")+"\nFeels: "+String(feels,1)+(fahrenheit?" F":" C")+"\nWind: "+String(wind,1)+" mph\nCode: "+String(wc);}
void connectWiFi(){if(!wifiSSID.length())return;WiFi.mode(WIFI_STA);WiFi.begin(wifiSSID.c_str(),wifiPassword.c_str());unsigned long t=millis();while(WiFi.status()!=WL_CONNECTED&&millis()-t<12000)delay(250);wifiOK=WiFi.status()==WL_CONNECTED;}

void setup(){
  Serial.begin(115200);delay(500);Serial.println("\n=== VORTEX 2.0 / CYD DISPLAY BOOT ===");
  initDisplay();touch.begin();touch.setRotation(1);
  header("VORTEX 2.0");tft.setTextColor(TFT_CYAN,TFT_BLACK);tft.setTextSize(2);tft.setCursor(8,60);tft.print("CYD DISPLAY OK");delay(700);
  tft.setCursor(8,60);tft.setTextColor(TFT_CYAN,TFT_BLACK);tft.setTextSize(2);tft.print("Starting...");
  Serial.printf("PSRAM: %u bytes\n",ESP.getPsramSize());sdOK=SD.begin(SD_CS);
  if(sdOK){if(!SD.exists("/vortex"))SD.mkdir("/vortex");if(!SD.exists("/vortex/knowledge"))SD.mkdir("/vortex/knowledge");loadConfig();}
  connectWiFi();bool m=loadCheckpoint();bool tokLoaded=m&&loadTokenizer();String status="PSRAM: "+String(ESP.getPsramSize()/1024)+" KB\nSD: "+String(sdOK?"OK":"MISSING")+"\nWi-Fi: "+String(wifiOK?"CONNECTED":"OFFLINE")+"\nNeural model: "+String(m?"LOADED":"NOT FOUND")+"\nTokenizer: "+String(tokLoaded?"LOADED":"NOT FOUND");textPage("VORTEX 2.0",status);
}
void loop(){
  if(touch.touched()){TS_Point p=touch.getPoint();int x=map(p.x,200,3900,0,320),y=map(p.y,200,3900,0,240);if(y>205&&x<105){textPage("WEATHER",weather());delay(500);}else if(y>205&&x<210){textPage("MODEL",model.loaded?"Neural transformer loaded from SD/PSRAM.\n\nOpen Serial at 115200 and type a prompt.":"Put /vortex/model.bin and tokenizer.bin on the SD card.");delay(500);}else if(y>205){textPage("MEMORY",knowledgeContext("Vortex").substring(0,650));delay(500);}}
  if(Serial.available()){String q=Serial.readStringUntil('\n');q.trim();if(q.length()){Serial.println("Vortex is thinking...");String ans=runVortex(q);Serial.println("VORTEX: "+ans);textPage("VORTEX",ans);}}
  delay(10);
}
