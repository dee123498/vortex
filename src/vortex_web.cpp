#include "vortex_web.h"
#include <WiFi.h>
#include <SD.h>

WebServer vortexWeb(80);
DNSServer vortexDNS;
static const byte DNS_PORT = 53;
static const char* AP_NAME = "VORTEX-AI";
static const char* AP_PASS = "Vortex208682De";

// vortex3_fixed.ino defines answer(String), not answer(const String&).
extern String answer(String q);
extern String purpose;
extern bool sdOK;
extern bool editPurpose;

// Keep this implementation in exactly one translation unit. The old
// vortex_web_integration.ino also defined it, which caused a duplicate-symbol
// linker error. The model is considered ready when the SD model exists.
bool vortexModelReady(){
  return sdOK && SD.exists("/vortex/model.bin");
}

static const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1"><title>Vortex AI</title><style>:root{--p:#101827;--q:#151f31;--l:#26344b;--t:#edf4ff;--m:#8ea1bd;--a:#63e6ff;--b:#8b7cff}*{box-sizing:border-box}body{margin:0;background:radial-gradient(circle at 20% 0,#142a4d,#070b14 42%);color:var(--t);font-family:system-ui,Segoe UI,Arial;min-height:100vh}.wrap{max-width:980px;margin:auto;padding:18px}.top{display:flex;align-items:center;gap:12px;padding:14px 16px;border:1px solid var(--l);background:#101827dd;border-radius:18px}.orb{width:44px;height:44px;border-radius:50%;background:radial-gradient(circle at 35% 30%,#fff,var(--a) 22%,#6572ff 55%,#251b6b);box-shadow:0 0 28px #4be5ff88}.brand{font-weight:800;font-size:20px}.sub{color:var(--m);font-size:12px}.grid{display:grid;grid-template-columns:1fr 260px;gap:14px;margin-top:14px}.chat{height:68vh;min-height:460px;display:flex;flex-direction:column;border:1px solid var(--l);background:#0a101bd9;border-radius:18px;overflow:hidden}.msgs{flex:1;overflow:auto;padding:18px}.msg{max-width:84%;padding:12px 14px;margin:9px 0;border-radius:15px;line-height:1.45;white-space:pre-wrap}.user{margin-left:auto;background:linear-gradient(135deg,#3346a8,#6548c9)}.bot{background:var(--q);border:1px solid var(--l)}.composer{display:flex;gap:9px;padding:12px;border-top:1px solid var(--l);background:var(--p)}textarea{flex:1;resize:none;border:1px solid var(--l);background:#080e18;color:var(--t);border-radius:13px;padding:12px;font:inherit;outline:none}button{border:0;border-radius:13px;padding:0 18px;background:linear-gradient(135deg,var(--a),var(--b));color:#06101b;font-weight:800;cursor:pointer}.side{border:1px solid var(--l);background:#101827d9;border-radius:18px;padding:15px;height:max-content}.pill{display:flex;justify-content:space-between;padding:9px 10px;background:#0a1220;border:1px solid var(--l);border-radius:10px;margin:8px 0;font-size:13px}.ok{color:#6dffb0}.hint{color:var(--m);font-size:12px;line-height:1.5}h3{margin:4px 0 12px}@media(max-width:760px){.grid{grid-template-columns:1fr}.side{display:none}.chat{height:78vh}}</style></head><body><div class="wrap"><div class="top"><div class="orb"></div><div><div class="brand">VORTEX AI</div><div class="sub">ESP32 CYD • OFFLINE ASSISTANT</div></div></div><div class="grid"><section class="chat"><div id="msgs" class="msgs"><div class="msg bot">Hello Dzavious. I'm Vortex. What are we working on?</div></div><div class="composer"><textarea id="q" rows="2" placeholder="Talk to Vortex..."></textarea><button onclick="send()">SEND</button></div></section><aside class="side"><h3>System</h3><div class="pill"><span>Wi-Fi</span><span class="ok">CONNECTED</span></div><div class="pill"><span>SD card</span><span id="sd">—</span></div><div class="pill"><span>Model</span><span id="model">—</span></div><div class="pill"><span>IP</span><span id="ip">—</span></div><h3>Commands</h3><div class="hint"><b>208682De</b> — change purpose<br><b>/learn</b> — learning<br><b>/remember</b> — memory<br><b>/memory</b> — view memory<br><b>/knowledge</b> — knowledge<br><b>/reload</b> — reload SD</div><h3>Purpose</h3><div id="purpose" class="hint">Loading...</div></aside></div></div><script>const m=document.getElementById('msgs'),q=document.getElementById('q');function add(t,c){let d=document.createElement('div');d.className='msg '+c;d.textContent=t;m.appendChild(d);m.scrollTop=m.scrollHeight}async function send(){let x=q.value.trim();if(!x)return;add(x,'user');q.value='';add('Thinking...','bot');let last=m.lastChild;try{let r=await fetch('/api/chat',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'q='+encodeURIComponent(x)});let j=await r.json();last.textContent=j.answer||'No response.'}catch(e){last.textContent='Connection error: '+e}}q.addEventListener('keydown',e=>{if(e.key==='Enter'&&!e.shiftKey){e.preventDefault();send()}});async function status(){try{let j=await(await fetch('/api/status')).json();document.getElementById('sd').textContent=j.sd?'READY':'MISSING';document.getElementById('model').textContent=j.model?'READY':'MISSING';document.getElementById('ip').textContent=j.ip;document.getElementById('purpose').textContent=j.purpose}catch(e){}}status();setInterval(status,5000);</script></body></html>)HTML";

static String esc(const String& s){String o;for(char c:s){if(c=='"'||c=='\\'){o+='\\';o+=c;}else if(c=='\n')o+="\\n";else if(c=='\r')o+="\\r";else o+=c;}return o;}
String vortexWebAnswer(const String& message){return answer(String(message));}
String vortexWebStatus(){return String("{\"sd\":")+(sdOK?"true":"false")+",\"model\":"+(vortexModelReady()?"true":"false")+",\"ip\":\""+WiFi.softAPIP().toString()+"\",\"purpose\":\""+esc(purpose)+"\"}");}
String vortexWebPurpose(){return purpose;}
void vortexWebHandleCommand(const String& message){answer(String(message));}

void vortexWebBegin(){
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);
  WiFi.softAPConfig(IPAddress(192,168,4,1),IPAddress(192,168,4,1),IPAddress(255,255,255,0));
  WiFi.softAP(AP_NAME,AP_PASS,1,false,4);
  delay(100);
  vortexDNS.start(DNS_PORT,"*",WiFi.softAPIP());
  vortexWeb.on("/",HTTP_GET,[](){vortexWeb.send_P(200,"text/html",INDEX_HTML);});
  vortexWeb.on("/generate_204",HTTP_GET,[](){vortexWeb.send_P(200,"text/html",INDEX_HTML);});
  vortexWeb.on("/hotspot-detect.html",HTTP_GET,[](){vortexWeb.send_P(200,"text/html",INDEX_HTML);});
  vortexWeb.on("/api/status",HTTP_GET,[](){vortexWeb.send(200,"application/json",vortexWebStatus());});
  vortexWeb.on("/api/chat",HTTP_POST,[](){
    String q=vortexWeb.arg("q"); q.trim(); String ans;
    if(q=="208682De"){
      editPurpose=true; ans="What is my purpose?";
    }else if(editPurpose&&q.length()){
      purpose=q; editPurpose=false;
      if(sdOK){File f=SD.open("/vortex/purpose.txt",FILE_WRITE);if(f){f.print(purpose);f.close();}}
      ans="My purpose has been updated.";
    }else ans=q.length()?answer(String(q)):"Please type a message.";
    vortexWeb.send(200,"application/json",String("{\"answer\":\"")+esc(ans)+"\"}");
  });
  vortexWeb.onNotFound([](){vortexWeb.send_P(200,"text/html",INDEX_HTML);});
  vortexWeb.begin();
  Serial.print("Vortex hotspot: ");Serial.println(AP_NAME);
  Serial.print("Vortex Wi-Fi IP: ");Serial.println(WiFi.softAPIP());
  Serial.println("Open http://192.168.4.1");
}
void vortexWebLoop(){vortexDNS.processNextRequest();vortexWeb.handleClient();}
