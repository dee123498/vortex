#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <SD.h>
#include <FS.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* VORTEX 2.0 — ESP32-S3 CYD + PSRAM
   Single source file. Model: llama2.c-style float32 checkpoint.
   SD: /vortex/model.bin, /vortex/tokenizer.bin, /vortex/config.txt
   Optional knowledge: /vortex/knowledge/*.txt or *.md
   No AI API key is used. Weather uses Open-Meteo when Wi-Fi is available.

   This is a genuine transformer inference path, replacing the old keyword
   responder. It is derived from the architecture already present in the
   Vortex repository and the ESP32-S3 llama2.c-style implementation used as
   the reference for the checkpoint layout and two-core/S3 optimization path.
*/

#define SD_CS 5
#define TOUCH_CS 33
#define TOUCH_IRQ 36
#define TFT_BL 21
#define MODEL_PATH "/vortex/model.bin"
#define TOKENIZER_PATH "/vortex/tokenizer.bin"
#define DEFAULT_STEPS 96

TFT_eSPI tft;
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

String wifiSSID, wifiPassword, locationName="My Location";
float latitude=41.1170f, longitude=-88.2217f;
bool fahrenheit=true, wifiOK=false, sdOK=false;

// ---------- model structures ----------
typedef struct { int vocab_size, dim, hidden_dim, n_layers, n_heads, n_kv_heads, seq_len; } Config;
typedef struct { char **vocab; float *scores; int vocab_size; unsigned int max_token_length; } Tokenizer;
typedef struct {
  float *token_embedding_table,*rms_att_weight,*wq,*wk,*wv,*wo,*rms_ffn_weight,*w1,*w2,*w3,*rms_final_weight,*wcls;
} Weights;
typedef struct {
  float *x,*xb,*xb2,*hb,*hb2,*q,*k,*v,*att,*logits,*key_cache,*value_cache;
} State;
struct Model { Config p{}; Weights w{}; State s{}; uint8_t *blob=nullptr; size_t bytes=0; bool loaded=false; } model;
Tokenizer tok{};

void *pa(size_t n){ void *p=ps_malloc(n); return p?p:malloc(n); }

// ---------- UI ----------
void page(const String &title,const String &body){
  tft.fillScreen(TFT_BLACK); tft.fillRect(0,0,320,42,TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE,TFT_DARKGREY); tft.setTextSize(2); tft.setCursor(7,12); tft.print(title);
  tft.setTextSize(1); tft.setCursor(240,15); tft.print(wifiOK?"ONLINE":"OFFLINE");
  tft.setTextColor(TFT_WHITE,TFT_BLACK); int y=56; String line;
  for(size_t i=0;i<body.length()&&y<235;i++){char c=body[i];if(c=='\n'||line.length()>48){tft.setCursor(6,y);tft.print(line);y+=12;line="";if(c!='\n')line+=c;}else line+=c;}
  if(line.length()&&y<235){tft.setCursor(6,y);tft.print(line);}
}

String readSD(const char *path){ if(!sdOK)return ""; File f=SD.open(path,FILE_READ);if(!f)return "";String s=f.readString();f.close();return s; }
void loadConfig(){
  if(!sdOK)return; if(!SD.exists("/vortex"))SD.mkdir("/vortex");
  String w=readSD("/vortex/wifi.txt"); int p=w.indexOf('\n');
  if(p>=0){wifiSSID=w.substring(0,p);wifiSSID.trim();wifiPassword=w.substring(p+1);wifiPassword.trim();}
  String c=readSD("/vortex/config.txt"); int i=c.indexOf("latitude=");if(i>=0)latitude=c.substring(i+9).toFloat();
  i=c.indexOf("longitude=");if(i>=0)longitude=c.substring(i+10).toFloat();
  i=c.indexOf("location=");if(i>=0){locationName=c.substring(i+9);locationName.trim();}
  i=c.indexOf("fahrenheit=");if(i>=0)fahrenheit=c.substring(i+11).toInt()!=0;
}

void connectWiFi(){
  if(!wifiSSID.length())return; WiFi.mode(WIFI_STA);WiFi.begin(wifiSSID.c_str(),wifiPassword.c_str());
  unsigned long st=millis();while(WiFi.status()!=WL_CONNECTED&&millis()-st<12000)delay(250);wifiOK=WiFi.status()==WL_CONNECTED;
}

// ---------- checkpoint loader ----------
bool allocState(){
  int d=model.p.dim,h=model.p.hidden_dim,L=model.p.n_layers,heads=model.p.n_heads,kv=model.p.n_kv_heads,seq=model.p.seq_len;
  int kvd=d*kv/heads;
  model.s.x=(float*)pa(d*4);model.s.xb=(float*)pa(d*4);model.s.xb2=(float*)pa(d*4);
  model.s.hb=(float*)pa(h*4);model.s.hb2=(float*)pa(h*4);model.s.q=(float*)pa(d*4);model.s.k=(float*)pa(kvd*4);model.s.v=(float*)pa(kvd*4);
  model.s.att=(float*)pa((size_t)heads*seq*4);model.s.logits=(float*)pa((size_t)model.p.vocab_size*4);
  model.s.key_cache=(float*)pa((size_t)L*seq*kvd*4);model.s.value_cache=(float*)pa((size_t)L*seq*kvd*4);
  return model.s.x&&model.s.xb&&model.s.xb2&&model.s.hb&&model.s.hb2&&model.s.q&&model.s.k&&model.s.v&&model.s.att&&model.s.logits&&model.s.key_cache&&model.s.value_cache;
}

bool loadModel(){
  if(!sdOK)return false;File f=SD.open(MODEL_PATH,FILE_READ);if(!f)return false;model.bytes=f.size();
  if(model.bytes<sizeof(Config)){f.close();return false;}model.blob=(uint8_t*)pa(model.bytes);if(!model.blob){f.close();return false;}
  if(f.read(model.blob,model.bytes)!=model.bytes){f.close();return false;}f.close();
  int rawV=0;memcpy(&rawV,model.blob,4); // llama2.c uses negative vocab_size to mean untied classifier weights
  memcpy(&model.p,model.blob,sizeof(Config));model.p.vocab_size=abs(model.p.vocab_size);
  if(model.p.dim<=0||model.p.hidden_dim<=0||model.p.n_layers<=0||model.p.n_heads<=0||model.p.n_kv_heads<=0||model.p.vocab_size<=0||model.p.seq_len<=0)return false;
  float *q=(float*)(model.blob+sizeof(Config));int d=model.p.dim,h=model.p.hidden_dim,heads=model.p.n_heads,kv=model.p.n_kv_heads,L=model.p.n_layers;int hs=d/heads;
  model.w.token_embedding_table=q;q+=(size_t)model.p.vocab_size*d;
  model.w.rms_att_weight=q;q+=(size_t)L*d;model.w.wq=q;q+=(size_t)L*d*d;model.w.wk=q+=(size_t)0; // replaced below
  // Rewind from known embedding/rms/wq position to avoid chained-assignment ambiguity.
  q=(float*)(model.blob+sizeof(Config));q+=(size_t)model.p.vocab_size*d;q+=(size_t)L*d;
  model.w.wq=q;q+=(size_t)L*d*(heads*hs);model.w.wk=q;q+=(size_t)L*d*(kv*hs);model.w.wv=q;q+=(size_t)L*d*(kv*hs);model.w.wo=q+=(size_t)0;
  q=(float*)(model.blob+sizeof(Config));q+=(size_t)model.p.vocab_size*d;q+=(size_t)L*d;q+=(size_t)L*d*d;q+=(size_t)L*d*(kv*hs);q+=(size_t)L*d*(kv*hs);model.w.wo=q;q+=(size_t)L*(heads*hs)*d;
  model.w.rms_ffn_weight=q;q+=(size_t)L*d;model.w.w1=q;q+=(size_t)L*d*h;model.w.w2=q+=(size_t)0;
  q=(float*)(model.blob+sizeof(Config));q+=(size_t)model.p.vocab_size*d;q+=(size_t)L*d;q+=(size_t)L*d*d;q+=(size_t)L*d*(kv*hs);q+=(size_t)L*d*(kv*hs);q+=(size_t)L*d*d;q+=(size_t)L*d;q+=(size_t)L*d*h;
  model.w.w2=q;q+=(size_t)L*h*d;model.w.w3=q+=(size_t)L*d*h;model.w.rms_final_weight=q+=(size_t)0;
  q=(float*)(model.blob+sizeof(Config));q+=(size_t)model.p.vocab_size*d;q+=(size_t)L*d;q+=(size_t)L*d*d;q+=(size_t)L*d*(kv*hs);q+=(size_t)L*d*(kv*hs);q+=(size_t)L*d*d;q+=(size_t)L*d;q+=(size_t)L*d*h;q+=(size_t)L*h*d;q+=(size_t)L*d*h;model.w.rms_final_weight=q;q+=d;
  q+=(size_t)model.p.seq_len*hs/2;q+=(size_t)model.p.seq_len*hs/2;model.w.wcls=rawV>0?model.w.token_embedding_table:q;
  if(!allocState())return false;memset(model.s.key_cache,0,(size_t)L*model.p.seq_len*(d*kv/heads)*4);memset(model.s.value_cache,0,(size_t)L*model.p.seq_len*(d*kv/heads)*4);model.loaded=true;return true;
}

// ---------- tokenizer ----------
bool loadTokenizer(){
  if(!sdOK||!model.loaded)return false;File f=SD.open(TOKENIZER_PATH,FILE_READ);if(!f)return false;uint32_t ml=0;if(f.read((uint8_t*)&ml,4)!=4){f.close();return false;}
  tok.vocab_size=model.p.vocab_size;tok.max_token_length=ml;tok.vocab=(char**)pa((size_t)tok.vocab_size*sizeof(char*));tok.scores=(float*)pa((size_t)tok.vocab_size*4);if(!tok.vocab||!tok.scores)return false;
  for(int i=0;i<tok.vocab_size;i++){float score;uint32_t len;if(f.read((uint8_t*)&score,4)!=4||f.read((uint8_t*)&len,4)!=4)return false;tok.scores[i]=score;tok.vocab[i]=(char*)pa(len+1);if(!tok.vocab[i])return false;if(f.read((uint8_t*)tok.vocab[i],len)!=len)return false;tok.vocab[i][len]=0;}f.close();return true;
}

// ---------- transformer ----------
float dotp(const float*a,const float*b,int n){float s=0;for(int i=0;i<n;i++)s+=a[i]*b[i];return s;}
void rms(float*o,const float*x,const float*w,int n){float s=0;for(int i=0;i<n;i++)s+=x[i]*x[i];s=1.0f/sqrtf(s/n+1e-5f);for(int i=0;i<n;i++)o[i]=w[i]*x[i]*s;}
void sm(float*x,int n){float m=x[0];for(int i=1;i<n;i++)if(x[i]>m)m=x[i];float s=0;for(int i=0;i<n;i++){x[i]=expf(x[i]-m);s+=x[i];}for(int i=0;i<n;i++)x[i]/=s;}
void mm(float*out,const float*x,const float*w,int n,int d){for(int i=0;i<d;i++)out[i]=dotp(w+(size_t)i*n,x,n);}
float silu(float x){return x/(1.0f+expf(-x));}
float*forward(int token,int pos){
  Config*p=&model.p;Weights*w=&model.w;State*s=&model.s;int d=p->dim,h=p->hidden_dim,hs=d/p->n_heads,kvd=d*p->n_kv_heads/p->n_heads,kvm=p->n_heads/p->n_kv_heads;
  memcpy(s->x,w->token_embedding_table+(size_t)token*d,d*4);
  for(int l=0;l<p->n_layers;l++){
    rms(s->xb,s->x,w->rms_att_weight+(size_t)l*d,d);mm(s->q,s->xb,w->wq+(size_t)l*d*d,d,d);mm(s->k,s->xb,w->wk+(size_t)l*d*kvd,d,kvd);mm(s->v,s->xb,w->wv+(size_t)l*d*kvd,d,kvd);
    for(int hh=0;hh<p->n_heads;hh++)for(int i=0;i<hs;i+=2){float freq=1.0f/powf(10000.0f,(float)i/hs),a=pos*freq,c=cosf(a),si=sinf(a);float*q=s->q+hh*hs,q0=q[i],q1=q[i+1];q[i]=q0*c-q1*si;q[i+1]=q0*si+q1*c;if(hh<p->n_kv_heads){float*k=s->k+hh*hs,k0=k[i],k1=k[i+1];k[i]=k0*c-k1*si;k[i+1]=k0*si+k1*c;}}
    size_t off=(size_t)l*p->seq_len*kvd+(size_t)pos*kvd;memcpy(s->key_cache+off,s->k,kvd*4);memcpy(s->value_cache+off,s->v,kvd*4);size_t layer=(size_t)l*p->seq_len*kvd;
    for(int hh=0;hh<p->n_heads;hh++){float*q=s->q+hh*hs,*att=s->att+hh*p->seq_len;for(int t=0;t<=pos;t++)att[t]=dotp(q,s->key_cache+layer+(size_t)t*kvd+(hh/kvm)*hs,hs)/sqrtf((float)hs);sm(att,pos+1);float*xb=s->xb+hh*hs;memset(xb,0,hs*4);for(int t=0;t<=pos;t++){float*a=s->value_cache+layer+(size_t)t*kvd+(hh/kvm)*hs;for(int i=0;i<hs;i++)xb[i]+=att[t]*a[i];}}
    mm(s->xb2,s->xb,w->wo+(size_t)l*d*d,d,d);for(int i=0;i<d;i++)s->x[i]+=s->xb2[i];rms(s->xb,s->x,w->rms_ffn_weight+(size_t)l*d,d);mm(s->hb,s->xb,w->w1+(size_t)l*h*d,d,h);mm(s->hb2,s->xb,w->w3+(size_t)l*h*d,d,h);for(int i=0;i<h;i++)s->hb[i]=silu(s->hb[i])*s->hb2[i];mm(s->xb2,s->hb,w->w2+(size_t)l*d*h,h,d);for(int i=0;i<d;i++)s->x[i]+=s->xb2[i];
  }
  rms(s->x,s->x,w->rms_final_weight,d);mm(s->logits,s->x,w->wcls,d,p->vocab_size);return s->logits;
}

uint32_t rs=0xA53C9E11;uint32_t rnd(){rs^=rs<<13;rs^=rs>>17;rs^=rs<<5;return rs;}
int sample(float*x,int n){float temp=.72f;for(int i=0;i<n;i++)x[i]/=temp;sm(x,n);const int K=64;int id[K];float pr[K];int c=0;for(int i=0;i<n;i++){float p=x[i];if(p<=0)continue;if(c<K){int j=c++;while(j>0&&pr[j-1]<p){pr[j]=pr[j-1];id[j]=id[j-1];j--;}pr[j]=p;id[j]=i;}else if(p>pr[K-1]){int j=K-1;while(j>0&&pr[j-1]<p){pr[j]=pr[j-1];id[j]=id[j-1];j--;}pr[j]=p;id[j]=i;}}float total=0;for(int i=0;i<c;i++)total+=pr[i];float r=(rnd()/(float)UINT32_MAX)*total*.9f,s=0;for(int i=0;i<c;i++){s+=pr[i];if(s>=r)return id[i];}return id[0];}

int encode(const String&s,int*out,int cap){if(!tok.vocab)return 0;int n=0;for(size_t p=0;p<s.length()&&n<cap;){int best=-1,bl=0;for(int i=0;i<tok.vocab_size;i++){int l=strlen(tok.vocab[i]);if(l>bl&&p+l<=s.length()&&!strncmp(s.c_str()+p,tok.vocab[i],l)){best=i;bl=l;}}if(best>=0){out[n++]=best;p+=bl;}else out[n++]=(uint8_t)s[p++];}return n;}

String memory(const String&q){if(!sdOK||!SD.exists("/vortex/knowledge"))return"";File d=SD.open("/vortex/knowledge");if(!d)return"";String best="";int scoreBest=0;File f=d.openNextFile();while(f){if(!f.isDirectory()){String nm=f.name();if(nm.endsWith(".txt")||nm.endsWith(".md")){String s=f.readString(),lo=s;lo.toLowerCase();String qq=q;qq.toLowerCase();int sc=0,p=0;while((p=lo.indexOf(qq,p))>=0){sc+=10;p+=max(1,(int)qq.length());}if(sc>scoreBest){scoreBest=sc;best=s.substring(0,min((int)s.length(),650));}}}f.close();f=d.openNextFile();}d.close();return best;}

String askVortex(const String&q){if(!model.loaded||!tok.vocab)return"Neural model/tokenizer not loaded. Put model.bin and tokenizer.bin in /vortex/.";int ids[256];String prompt="You are Vortex, a helpful offline AI assistant.\nUser: "+q+"\n";String mem=memory(q);if(mem.length())prompt+="Memory:\n"+mem+"\n";prompt+="Vortex:";int n=encode(prompt,ids,256);if(!n)return"Tokenizer failed.";int pos=min(n,model.p.seq_len-1);for(int i=0;i<pos;i++)forward(ids[i],i);int token=ids[pos-1];String out;unsigned long st=millis();for(int step=0;step<DEFAULT_STEPS&&pos+step<model.p.seq_len;step++){int next=sample(forward(token,pos+step),model.p.vocab_size);if(next<0||next>=tok.vocab_size)break;String piece=tok.vocab[next];out+=piece;token=next;if(piece.indexOf("</s>")>=0||piece.indexOf("<eos>")>=0||out.length()>700)break;}Serial.printf("generated in %.2fs\n",(millis()-st)/1000.0f);return out.length()?out:"(no output)";}

String weather(){if(!wifiOK)return"Offline — Wi-Fi is not connected.";HTTPClient h;String u=fahrenheit?"fahrenheit":"celsius";String url="https://api.open-meteo.com/v1/forecast?latitude="+String(latitude,5)+"&longitude="+String(longitude,5)+"&current=temperature_2m,apparent_temperature,wind_speed_10m,weather_code&temperature_unit="+u+"&wind_speed_unit=mph";h.begin(url);int c=h.GET();if(c!=200){h.end();return"Weather request failed.";}DynamicJsonDocument doc(8192);auto e=deserializeJson(doc,h.getString());h.end();if(e)return"Weather data invalid.";JsonObject x=doc["current"];return locationName+"\nTemp: "+String((float)(x["temperature_2m"]|0.0f),1)+(fahrenheit?" F":" C")+"\nFeels: "+String((float)(x["apparent_temperature"]|0.0f),1)+(fahrenheit?" F":" C")+"\nWind: "+String((float)(x["wind_speed_10m"]|0.0f),1)+" mph";}

void setup(){Serial.begin(115200);delay(300);pinMode(TFT_BL,OUTPUT);digitalWrite(TFT_BL,HIGH);tft.init();tft.setRotation(1);touch.begin();touch.setRotation(1);page("VORTEX 2.0","Booting neural AI...");Serial.printf("PSRAM %u KB\n",ESP.getPsramSize()/1024);sdOK=SD.begin(SD_CS);if(sdOK){if(!SD.exists("/vortex"))SD.mkdir("/vortex");if(!SD.exists("/vortex/knowledge"))SD.mkdir("/vortex/knowledge");loadConfig();}connectWiFi();bool m=loadModel(),tt=m&&loadTokenizer();page("VORTEX 2.0","PSRAM: "+String(ESP.getPsramSize()/1024)+" KB\nSD: "+String(sdOK?"OK":"MISSING")+"\nWi-Fi: "+String(wifiOK?"ONLINE":"OFFLINE")+"\nNeural model: "+String(m?"LOADED":"NOT FOUND")+"\nTokenizer: "+String(tt?"LOADED":"NOT FOUND")+"\n\nSerial: type a prompt.");}

void loop(){if(touch.touched()){TS_Point p=touch.getPoint();int x=map(p.x,200,3900,0,320),y=map(p.y,200,3900,0,240);if(y>200&&x<106)page("WEATHER",weather());else if(y>200&&x<212)page("MODEL",model.loaded?"Transformer running from PSRAM.\nUse USB Serial for chat.":"Missing /vortex/model.bin");else if(y>200)page("MEMORY",memory("Vortex"));delay(450);}if(Serial.available()){String q=Serial.readStringUntil('\n');q.trim();if(q.length()){Serial.println("Vortex thinking...");String a=askVortex(q);Serial.println("VORTEX: "+a);page("VORTEX",a);}}delay(10);}
