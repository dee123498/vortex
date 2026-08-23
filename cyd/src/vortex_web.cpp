#include "vortex_web.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <SD.h>

extern HardwareSerial PicoSerial;

static WebServer server(80);
static DNSServer dns;
static bool webReady = false;
static bool apReady = false;
static bool webWaiting = false;
static String webReply = "Ready for a conversation.";
static String webUser = "Guest";
static String webRole = "guest";
static String session;
static String apName = "VORTEX-AI";
static String apPassword = "VortexAI123!";
static String staIP = "";

static String htmlEscape(String s) {
  s.replace("&", "&amp;"); s.replace("<", "&lt;"); s.replace(">", "&gt;");
  s.replace("\"", "&quot;"); return s;
}

static String jsonEscape(String s) {
  s.replace("\\", "\\\\"); s.replace("\"", "\\\"");
  s.replace("\r", ""); s.replace("\n", "\\n"); return s;
}

static String makeToken() {
  uint32_t a = esp_random(), b = esp_random(), c = esp_random();
  char out[32]; snprintf(out, sizeof(out), "%08lx%08lx%08lx", (unsigned long)a, (unsigned long)b, (unsigned long)c);
  return String(out);
}

static bool authorized() {
  return session.length() && server.header("X-Vortex-Session") == session;
}

static void json(int code, const String &body) { server.send(code, "application/json", body); }

static void loadAPSettings() {
  if (!SD.exists("/vortex/ap.txt")) return;
  File f = SD.open("/vortex/ap.txt", FILE_READ);
  if (!f) return;
  String s = f.readStringUntil('\n');
  String p = f.readStringUntil('\n');
  f.close(); s.trim(); p.trim();
  if (s.length() >= 1 && s.length() <= 32) apName = s;
  if (p.length() >= 8 && p.length() <= 63) apPassword = p;
}

static void loadSTA() {
  if (!SD.exists("/vortex/wifi.txt")) return;
  File f = SD.open("/vortex/wifi.txt", FILE_READ);
  if (!f) return;
  String ssid = f.readStringUntil('\n');
  String pass = f.readString();
  f.close(); ssid.trim(); pass.trim();
  if (!ssid.length()) return;
  WiFi.begin(ssid.c_str(), pass.c_str());
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 8000) delay(50);
  if (WiFi.status() == WL_CONNECTED) staIP = WiFi.localIP().toString();
}

static const char PAGE[] PROGMEM = R"HTML(
<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1"><title>Vortex AI</title>
<style>
*{box-sizing:border-box}body{margin:0;background:radial-gradient(circle at 50% 35%,#182353,#070a18 62%);color:#eef3ff;font:14px Arial,Segoe UI,sans-serif}.app{min-height:100vh;display:grid;grid-template-columns:220px 1fr 270px}.side,.right{background:#0a0f27ee;padding:18px;border-color:#252d59}.side{border-right:1px solid #252d59}.right{border-left:1px solid #252d59}.brand{font-size:23px;font-weight:800;margin-bottom:20px;color:#14d8ff}.nav button,.control{display:block;width:100%;padding:11px;margin:4px 0;border:0;border-radius:10px;background:#101735;color:#aab5dd;text-align:left}.nav button:hover,.control:hover{background:#182455;color:#fff}.box,.metric{background:#101735;border:1px solid #252f60;border-radius:14px;padding:13px;margin-top:13px}.cyan{color:#14d8ff}.online{color:#55e4b0}.main{padding:20px 28px}.top{display:flex;justify-content:space-between}.layout{display:grid;grid-template-columns:170px 1fr;gap:18px;margin-top:25px}.metrics{display:grid;grid-template-columns:1fr 1fr;gap:9px}.metric label{display:block;font-size:10px;color:#14d8ff;text-transform:uppercase}.bar{height:5px;background:#29315b;border-radius:8px;margin-top:8px}.bar i{display:block;height:100%;background:linear-gradient(90deg,#14d8ff,#8b5cf6);border-radius:8px}.center{text-align:center}.orb{width:270px;height:270px;border-radius:50%;margin:5px auto;background:radial-gradient(circle,#14265d,#080e27 62%,#050815);border:2px solid #14d8ff99;box-shadow:0 0 45px #14d8ff44,inset 0 0 50px #8b5cf633}.chat{display:flex;gap:8px;background:#0c1331;border:1px solid #293466;border-radius:15px;padding:9px;margin:18px auto 0;max-width:700px}.chat input{flex:1;background:transparent;border:0;outline:0;color:#fff}.btn{border:0;border-radius:10px;padding:11px 14px;background:#1769ed;color:#fff}.mic{background:#172455}.login{position:fixed;inset:0;background:#000b;display:flex;align-items:center;justify-content:center;z-index:5}.card{width:360px;background:#101735;border:1px solid #354174;border-radius:18px;padding:24px}.card input{width:100%;padding:12px;margin:6px 0;background:#090f27;border:1px solid #2a3565;color:#fff;border-radius:10px}.hide{display:none!important}@media(max-width:1000px){.app{grid-template-columns:190px 1fr}.right{display:none}}@media(max-width:680px){.app{display:block}.side{display:none}.main{padding:14px}.layout{grid-template-columns:1fr}.orb{width:220px;height:220px}}
</style></head><body>
<div id="login" class="login"><div class="card"><h2>VORTEX <span class="cyan">AI</span> <small>V4</small></h2><p class="online">CYD Web Access</p><input id="name" placeholder="Your name"><select id="role" style="width:100%;padding:11px;background:#090f27;color:white;border:1px solid #2a3565;border-radius:10px"><option value="guest">Guest</option><option value="director">Director</option><option value="admin">Admin</option></select><input id="pass" type="password" placeholder="Password (required for Admin/Director)"><button class="btn" onclick="login()">Enter Vortex</button><p id="msg"></p><small>Connect to the Vortex Wi-Fi network, then open <b>192.168.4.1</b>.</small></div></div>
<div class="app"><aside class="side"><div class="brand">VORTEX AI <small>V4</small></div><div class="nav"><button>⌂ Dashboard</button><button>◌ Conversations</button><button>✦ Commands</button><button>◇ Knowledge</button><button>♡ Memory</button><button>▣ Media</button><button>♙ Users</button><button>⚙ Settings</button><button>◷ Logs</button></div><div class="box"><small>ACTIVE USER</small><div id="user">Guest</div><small id="roleText">GUEST</small></div><div class="box"><small>SYSTEM</small><p>AI <span class="online">Pico Online</span></p><p>SD <span id="sd" class="online">CYD</span></p><p>Wi-Fi <span class="online">AP</span></p></div></aside>
<main class="main"><div class="top"><b>AI Assistance › <span class="cyan">Vortex</span> <span class="online">● Online</span></b><span id="net">Wi-Fi AP</span></div><div class="layout"><div class="metrics"><div class="metric"><label>Knowledge</label><b>Ready</b><div class="bar"><i style="width:82%"></i></div></div><div class="metric"><label>Conversation</label><b>Ready</b><div class="bar"><i style="width:76%"></i></div></div><div class="metric"><label>ESP32</label><b>Ready</b><div class="bar"><i style="width:91%"></i></div></div><div class="metric"><label>Storage</label><b>SD</b><div class="bar"><i style="width:69%"></i></div></div><div class="metric"><label>Memory</label><b>Active</b><div class="bar"><i style="width:64%"></i></div></div><div class="metric"><label>Voice</label><b>Browser</b><div class="bar"><i style="width:88%"></i></div></div></div><div class="center"><div class="orb"></div><div class="cyan" id="greet">How can I help you?</div><div id="state">Ready</div><div style="font-size:34px">🎙</div><button class="btn mic" onclick="listenNow()">Talk to Vortex</button><div class="chat"><input id="q" placeholder="Type a message or speak..." onkeydown="if(event.key==='Enter')chat()"><button class="btn mic" onclick="listenNow()">🎙</button><button class="btn" onclick="chat()">➤</button></div></div></div></main>
<aside class="right"><div class="box"><b>VORTEX RESPONSE</b><p id="answer" class="cyan">Ready for a conversation.</p></div><div class="box"><b>CONTROL CENTER</b><button class="control" onclick="status()">⌁ System Status</button><button class="control" onclick="forget()">♡ Clear My Memory</button><button class="control" onclick="reboot()">↻ Restart Vortex</button></div><div class="box"><b>NETWORK</b><p id="network">VORTEX-AI<br>192.168.4.1</p><button class="btn" onclick="location.reload()">Sign out</button></div></aside></div>
<script>
let tok='',R='guest';async function login(){let r=await fetch('/api/login',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'name='+encodeURIComponent(name.value)+'&role='+role.value+'&password='+encodeURIComponent(pass.value)}),d=await r.json();if(!r.ok){msg.textContent=d.error;return}tok=d.token;R=d.role;document.getElementById('login').classList.add('hide');user.textContent=d.name;roleText.textContent=R.toUpperCase();greet.textContent='How can I help you today, '+d.name.split(' ')[0]+'?'}
async function chat(){let x=q.value.trim();if(!x||!tok)return;q.value='';state.textContent='Thinking...';let r=await fetch('/api/chat',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded','X-Vortex-Session':tok},body:'message='+encodeURIComponent(x)}),d=await r.json();answer.textContent=d.answer||d.error;state.textContent='Ready';if(d.answer)speak(d.answer)}
function speak(t){if(window.speechSynthesis){let s=new SpeechSynthesisUtterance(t);s.rate=1;speechSynthesis.cancel();speechSynthesis.speak(s)}}
function listenNow(){let S=window.SpeechRecognition||window.webkitSpeechRecognition;if(!S)return alert('Use Chrome or Edge for voice input.');let r=new S();r.lang='en-US';r.onstart=()=>state.textContent='Listening...';r.onresult=e=>{q.value=e.results[0][0].transcript;chat()};r.onerror=()=>state.textContent='Voice error';r.onend=()=>{if(state.textContent==='Listening...')state.textContent='Ready'};r.start()}
async function status(){let r=await fetch('/api/status');let d=await r.json();alert('Vortex: '+d.vortex+'\nPico: '+d.pico+'\nAP: '+d.ap+'\nSTA: '+d.sta)}
async function forget(){if(!tok)return;let r=await fetch('/api/forget',{method:'POST',headers:{'X-Vortex-Session':tok}});answer.textContent=(await r.json()).message}
async function reboot(){if(R!=='admin')return alert('Admin only');if(confirm('Restart the CYD?'))fetch('/api/admin/reboot',{method:'POST',headers:{'X-Vortex-Session':tok}})}
</script></body></html>
)HTML";

static void root() { server.send_P(200, "text/html", PAGE); }

static void login() {
  String name = server.arg("name"); String role = server.arg("role"); String pass = server.arg("password");
  name.trim(); if (!name.length()) name = "Guest";
  if (role == "admin") {
    if (pass != "vortex") return json(401, "{\"error\":\"Invalid admin password\"}");
  } else if (role == "director") {
    if (pass != "director") return json(401, "{\"error\":\"Invalid director password\"}");
  } else role = "guest";
  webUser = name; webRole = role; session = makeToken();
  json(200, "{\"token\":\"" + session + "\",\"role\":\"" + role + "\",\"name\":\"" + jsonEscape(name) + "\"}");
}

static void chat() {
  if (!authorized()) return json(401, "{\"error\":\"Session expired\"}");
  String q = server.arg("message"); q.trim();
  if (!q.length()) return json(400, "{\"error\":\"Empty message\"}");
  webWaiting = true; webReply = "Vortex is thinking...";
  PicoSerial.println("USER\tweb_" + webUser + "\t" + webUser);
  PicoSerial.println("PROMPT\t" + q);
  unsigned long start = millis();
  while (webWaiting && millis() - start < 5000) { vortexWebLoop(); delay(2); }
  if (webWaiting) { webWaiting = false; webReply = "The Pico did not answer within 5 seconds."; }
  json(200, "{\"answer\":\"" + jsonEscape(webReply) + "\"}");
}

static void forget() {
  if (!authorized()) return json(401, "{\"error\":\"Session expired\"}");
  PicoSerial.println("USER\tweb_" + webUser + "\t" + webUser);
  PicoSerial.println("PROMPT\tforget memory");
  json(200, "{\"message\":\"Memory clear requested.\"}");
}

static void status() {
  String sta = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "offline";
  json(200, "{\"vortex\":\"online\",\"pico\":\"uart\",\"ap\":\"" + jsonEscape(WiFi.softAPIP().toString()) + "\",\"sta\":\"" + jsonEscape(sta) + "\"}");
}

static void reboot() {
  if (!authorized() || webRole != "admin") return json(403, "{\"error\":\"Admin only\"}");
  json(200, "{\"message\":\"Restarting\"}"); delay(150); ESP.restart();
}

static void captive() { server.sendHeader("Location", "http://192.168.4.1/", true); server.send(302, "text/plain", "Redirecting to Vortex"); }

void vortexWebBegin() {
  loadAPSettings();
  WiFi.mode(WIFI_AP_STA);
  apReady = WiFi.softAP(apName.c_str(), apPassword.c_str());
  dns.start(53, "*", WiFi.softAPIP());
  loadSTA();

  const char *headers[] = {"X-Vortex-Session"};
  server.collectHeaders(headers, 1);
  server.on("/", HTTP_GET, root);
  server.on("/api/login", HTTP_POST, login);
  server.on("/api/chat", HTTP_POST, chat);
  server.on("/api/forget", HTTP_POST, forget);
  server.on("/api/status", HTTP_GET, status);
  server.on("/api/admin/reboot", HTTP_POST, reboot);
  server.onNotFound(captive);
  server.begin();
  webReady = true;

  Serial.printf("VORTEX Wi-Fi AP: %s / %s\n", apName.c_str(), apPassword.c_str());
  Serial.printf("VORTEX UI: http://%s\n", WiFi.softAPIP().toString().c_str());
  if (staIP.length()) Serial.printf("VORTEX STA UI: http://%s\n", staIP.c_str());
}

void vortexWebLoop() {
  if (!webReady) return;
  dns.processNextRequest();
  server.handleClient();
}

void vortexWebOnPicoLine(const String &line) {
  if (!webWaiting) return;
  if (line == "END") webWaiting = false;
  else if (line.startsWith("TEXT\t")) webReply = line.substring(5);
  else if (line.startsWith("ERROR\t")) { webReply = "Pico error: " + line.substring(6); webWaiting = false; }
}
