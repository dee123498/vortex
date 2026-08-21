#include "vortex_web.h"
#include "tiny_llm.h"
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <esp_system.h>

extern TinyLLM llm;

static WebServer server(80);
static Preferences prefs;
static String adminHash;
static String directorHash;
static String directorName;
static String purposeText;
static String sessionToken;
static String sessionRole;
static String sessionUser;
static unsigned long sessionUntil = 0;

static String hashText(const String &s) {
  // Lightweight deterministic hash for local device access control.
  // This is not intended for internet-facing authentication.
  uint32_t h1 = 2166136261u, h2 = 0x9e3779b9u;
  for (size_t i = 0; i < s.length(); ++i) {
    uint8_t c = (uint8_t)s[i];
    h1 ^= c; h1 *= 16777619u;
    h2 ^= (h1 >> 13) + c; h2 *= 0x85ebca6bu;
  }
  char out[25];
  snprintf(out, sizeof(out), "%08lx%08lx%08lx", (unsigned long)h1,
           (unsigned long)h2, (unsigned long)(h1 ^ h2 ^ 0xA5A55A5Au));
  return String(out);
}

static String randomToken(size_t n = 24) {
  const char *abc = "ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz23456789";
  String r;
  r.reserve(n);
  for (size_t i = 0; i < n; ++i) r += abc[esp_random() % 57];
  return r;
}

static bool authed(const String &required = "") {
  if (sessionToken.length() == 0 || millis() > sessionUntil) return false;
  if (server.header("X-Vortex-Session") != sessionToken) return false;
  return required.length() == 0 || sessionRole == required;
}

static String jsonEscape(String s) {
  s.replace("\\", "\\\\"); s.replace("\"", "\\\"");
  s.replace("\n", "\\n"); s.replace("\r", "\\r");
  return s;
}

static void sendJson(int code, const String &body) {
  server.send(code, "application/json", body);
}

static const char INDEX_HTML[] PROGMEM = R"VORTEXHTML(
<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Vortex AI</title>
<style>
:root{--bg:#080b1d;--panel:#101633;--panel2:#151b42;--cyan:#16d8ff;--purple:#8b5cf6;--text:#eef3ff;--muted:#8e9ac5;--green:#22c55e;--red:#ef4444}
*{box-sizing:border-box}body{margin:0;background:radial-gradient(circle at 55% 45%,#152052 0,#090c21 48%,#050611 100%);color:var(--text);font-family:Inter,Segoe UI,Arial,sans-serif;min-height:100vh}
button,input{font:inherit}button{cursor:pointer;border:0}.app{display:grid;grid-template-columns:250px 1fr 300px;min-height:100vh}.side{background:#0b0f28dd;border-right:1px solid #252d59;padding:22px 16px}.brand{font-size:23px;font-weight:800;color:#fff;margin:0 0 25px}.brand b{color:var(--cyan)}.nav button{width:100%;padding:13px 14px;margin:4px 0;background:transparent;color:#aab4da;text-align:left;border-radius:12px}.nav button:hover,.nav .on{background:#15204b;color:#fff}.role{margin-top:22px;padding:14px;border:1px solid #26315f;border-radius:14px;background:#0e1430}.role small{color:var(--muted)}.main{padding:22px 28px;position:relative}.top{display:flex;justify-content:space-between;align-items:center;margin-bottom:16px}.online{color:#7ee7ff}.grid{display:grid;grid-template-columns:190px 1fr;gap:20px}.metrics{display:grid;grid-template-columns:1fr 1fr;gap:12px}.metric{padding:15px;background:#11183a;border:1px solid #202957;border-radius:14px}.metric label{font-size:10px;color:var(--cyan);text-transform:uppercase}.bar{height:6px;background:#29315b;border-radius:8px;margin-top:12px;overflow:hidden}.bar i{display:block;height:100%;background:linear-gradient(90deg,var(--cyan),var(--purple));border-radius:8px}.orbwrap{text-align:center;padding:10px}.orb{width:280px;height:280px;margin:8px auto;border-radius:50%;background:radial-gradient(circle,#12245d 0,#08102b 57%,#07101e 72%);border:2px solid #15d8ff88;box-shadow:0 0 40px #0dd7ff55, inset 0 0 50px #623bff33;display:flex;align-items:center;justify-content:center;position:relative}.orb:after{content:'';width:100%;height:2px;background:linear-gradient(90deg,transparent,#13d9ff,#a855f7,#13d9ff,transparent);box-shadow:0 0 15px #13d9ff;animation:pulse 1.2s infinite alternate}@keyframes pulse{from{opacity:.25;transform:scaleX(.7)}to{opacity:1;transform:scaleX(1.1)}}.listen{font-size:26px;margin-top:8px}.sub{color:#1ad8ff;margin:5px}.mic{font-size:35px;color:var(--cyan);margin:8px}.chat{margin-top:18px;background:#0d1433;border:1px solid #283363;border-radius:16px;padding:12px;display:flex;gap:10px}.chat input{flex:1;background:transparent;border:0;outline:0;color:#fff}.send,.voice{width:45px;height:42px;border-radius:12px;background:#1767e9;color:#fff}.voice{background:#17265b}.right{padding:22px 18px;background:#0b0f28dd;border-left:1px solid #252d59}.card{background:#101733;border:1px solid #242d5b;border-radius:15px;padding:16px;margin-bottom:14px}.card h3{font-size:13px;margin:0 0 13px}.result{color:#12d8ff}.video{padding:10px;background:#151d42;border-radius:10px;margin-top:8px}.adminbox button{display:block;width:100%;margin:8px 0;padding:12px;border-radius:10px;background:#172052;color:#dfe6ff;text-align:left}.footer{position:absolute;left:28px;right:28px;bottom:14px;display:flex;justify-content:space-between;color:#8290bd;font-size:12px}.modal{position:fixed;inset:0;background:#03040dcc;display:flex;align-items:center;justify-content:center;z-index:10}.modal.hide{display:none}.login{width:360px;background:#101735;border:1px solid #344078;border-radius:18px;padding:24px;box-shadow:0 0 50px #000}.login h2{margin-top:0}.login input{width:100%;padding:12px;margin:7px 0;background:#0a1028;border:1px solid #2a3566;border-radius:10px;color:#fff}.login button{width:100%;padding:12px;margin-top:8px;border-radius:10px;background:#1767e9;color:#fff}.tabs{display:flex;gap:8px;margin-bottom:10px}.tabs button{flex:1;background:#172052;color:#b8c4ed;padding:9px;border-radius:9px}.notice{font-size:12px;color:#9eacd8;margin-top:8px}.hidden{display:none!important}@media(max-width:1050px){.app{grid-template-columns:210px 1fr}.right{display:none}}@media(max-width:700px){.app{display:block}.side{display:none}.main{padding:14px}.grid{grid-template-columns:1fr}.orb{width:220px;height:220px}.footer{position:static;margin-top:20px}}
</style></head><body>
<div id="login" class="modal"><div class="login"><h2>VORTEX <span style="color:#16d8ff">AI</span></h2><div class="tabs"><button onclick="showLogin('admin')">Admin</button><button onclick="showLogin('director')">Director</button><button onclick="showLogin('guest')">Guest</button></div><div id="loginFields"><input id="user" placeholder="Username"><input id="pass" type="password" placeholder="Password"><button onclick="login()">Sign in</button></div><div id="guestFields" class="hidden"><input id="guestName" placeholder="Your name"><button onclick="guestLogin()">Enter as Guest</button></div><div id="loginMsg" class="notice"></div></div></div>
<div class="app"><aside class="side"><div class="brand"><b>VORTEX</b> AI <small>V4</small></div><div class="nav"><button class="on">⌂ &nbsp; Dashboard</button><button>◌ &nbsp; Conversations</button><button id="cmdNav">✦ &nbsp; Commands</button><button>◇ &nbsp; Knowledge</button><button>♡ &nbsp; Memory</button><button>▣ &nbsp; Media</button><button id="usersNav">♙ &nbsp; Users</button><button id="settingsNav">⚙ &nbsp; Settings</button><button>◷ &nbsp; Logs</button></div><div class="role"><small>ACTIVE ROLE</small><div id="role">Guest</div><small id="who">Guest User</small></div><div class="role"><small>SYSTEM STATUS</small><p>Model <span class="online">Online</span></p><p>Voice <span class="online">Browser</span></p><p>Storage <span class="online">SD</span></p><p>Network <span class="online">Local AP</span></p></div></aside>
<main class="main"><div class="top"><div><b>AI Assistance</b> <span style="color:#16d8ff">› Vortex</span> <span class="online">● Online</span></div><div>☼ &nbsp; ♧ &nbsp; 🎙️</div></div><div class="grid"><div class="metrics"><div class="metric"><label>Knowledge</label><b>Online</b><div class="bar"><i style="width:82%"></i></div></div><div class="metric"><label>Conversation</label><b>Ready</b><div class="bar"><i style="width:76%"></i></div></div><div class="metric"><label>ESP32</label><b>Ready</b><div class="bar"><i style="width:91%"></i></div></div><div class="metric"><label>SD Storage</label><b>128 GB</b><div class="bar"><i style="width:69%"></i></div></div><div class="metric"><label>Memory</label><b>Active</b><div class="bar"><i style="width:64%"></i></div></div><div class="metric"><label>Voice</label><b>Ready</b><div class="bar"><i style="width:88%"></i></div></div></div><div class="orbwrap"><div class="orb"></div><div class="sub" id="greet">How can I help you today?</div><div class="listen" id="listen">Ready</div><div class="mic">🎙</div><button class="voice" onclick="listenNow()">Hold to Talk</button><div class="chat"><input id="msg" placeholder="Type a message or speak..."><button class="voice" onclick="listenNow()">🎙</button><button class="send" onclick="sendMsg()">➤</button></div></div></div><div class="footer"><span>Vortex local assistant</span><span id="purpose"></span></div></main>
<aside class="right"><div class="card"><h3>SEARCHED BY COMMAND</h3><div class="result">Vortex</div><p id="searchText">Ready for a conversation.</p></div><div class="card"><h3>VOICE</h3><p>Speech recognition: <span class="result" id="speechState">Available</span></p><p>Speech output: <span class="result">Available</span></p></div><div class="card adminbox" id="adminBox"><h3>CONTROL CENTER</h3><button onclick="changePurpose()">⚙ System Purpose</button><button onclick="runStatus()">⌘ Run Command</button><button onclick="manageUsers()">♙ User Management</button><button onclick="commandCenter()">✦ Command Center</button><button onclick="settings()">⚙ System Settings</button></div><div class="card"><h3>ACTIVE USER</h3><div id="active">Guest</div><button style="margin-top:12px;padding:9px;border-radius:9px;background:#19234d;color:#fff" onclick="logout()">Sign out</button></div></aside></div>
<script>
let role='guest',token='',loginMode='admin';
function showLogin(m){loginMode=m;document.getElementById('loginFields').classList.toggle('hidden',m==='guest');document.getElementById('guestFields').classList.toggle('hidden',m!=='guest');document.getElementById('loginMsg').textContent=m==='director'?'First-time director accounts will ask for your name after sign-in.':''}
async function login(){let u=document.getElementById('user').value,p=document.getElementById('pass').value;let r=await fetch('/api/login',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'role='+encodeURIComponent(loginMode)+'&username='+encodeURIComponent(u)+'&password='+encodeURIComponent(p)});let d=await r.json();if(!r.ok){document.getElementById('loginMsg').textContent=d.error||'Login failed';return}token=d.token;role=d.role;finish(d)}
async function guestLogin(){let n=document.getElementById('guestName').value.trim()||'Guest User';let r=await fetch('/api/guest',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'name='+encodeURIComponent(n)});let d=await r.json();token=d.token;role='guest';finish(d)}
function finish(d){document.getElementById('login').classList.add('hide');document.getElementById('role').textContent=role.toUpperCase();document.getElementById('who').textContent=d.name||role;document.getElementById('active').textContent=d.name||role;document.getElementById('purpose').textContent=d.purpose||'';document.getElementById('greet').textContent='How can I help you today, '+(d.name||'').split(' ')[0]+'?';document.getElementById('adminBox').classList.toggle('hidden',role==='guest');document.getElementById('settingsNav').classList.toggle('hidden',role==='guest');}
async function sendMsg(){let i=document.getElementById('msg'),q=i.value.trim();if(!q)return;i.value='';document.getElementById('listen').textContent='Thinking...';let r=await fetch('/api/chat',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded','X-Vortex-Session':token},body:'message='+encodeURIComponent(q)});let d=await r.json();let a=d.answer||d.error||'No response';document.getElementById('searchText').textContent=a;document.getElementById('listen').textContent='Ready';speak(a)}
function speak(t){if('speechSynthesis' in window){speechSynthesis.cancel();let u=new SpeechSynthesisUtterance(t);u.rate=.98;u.pitch=1;speechSynthesis.speak(u)}}
function listenNow(){let SR=window.SpeechRecognition||window.webkitSpeechRecognition;if(!SR){alert('Speech recognition is not supported by this browser. Use Chrome or Edge.');return}let r=new SR();r.lang='en-US';r.interimResults=false;r.onstart=()=>document.getElementById('listen').textContent='Listening...';r.onresult=e=>{document.getElementById('msg').value=e.results[0][0].transcript;sendMsg()};r.onerror=e=>document.getElementById('listen').textContent='Voice error: '+e.error;r.onend=()=>{if(document.getElementById('listen').textContent==='Listening...')document.getElementById('listen').textContent='Ready'};r.start()}
async function changePurpose(){if(role!=='admin')return alert('Admin only');let p=prompt('New Vortex purpose:');if(!p)return;let r=await fetch('/api/admin/purpose',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded','X-Vortex-Session':token},body:'purpose='+encodeURIComponent(p)});let d=await r.json();alert(d.message||d.error);if(r.ok)document.getElementById('purpose').textContent=p}
async function runStatus(){if(role!=='admin')return alert('Admin only');let r=await fetch('/api/admin/command',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded','X-Vortex-Session':token},body:'command=status'});let d=await r.json();alert(d.result||d.error)}
function manageUsers(){alert(role==='admin'?'User management: Admin can configure the Director account. Guest users are chat-only. Use the Director setup command to assign the director password.':'Not permitted')}
function commandCenter(){if(role!=='admin')return;alert('Safe command center: status, reboot, purpose. Arbitrary shell/code execution is intentionally not exposed.')}
function settings(){if(role==='guest')return alert('Guests cannot change settings.');alert(role==='admin'?'Admin settings are available in the control center.':'Director settings are available, but system ownership and command permissions remain Admin-only.')}
function logout(){token='';location.reload()}
showLogin('admin');
</script></body></html>
)VORTEXHTML";

static void handleRoot(){ server.send_P(200,"text/html",INDEX_HTML); }

static void loadConfig(){
  prefs.begin("vortex-auth", false);
  adminHash=prefs.getString("admin", "");
  directorHash=prefs.getString("director", "");
  directorName=prefs.getString("dname", "");
  purposeText=prefs.getString("purpose", "Help Dezavious Ojelade with tasks, programming, learning, engineering, and everyday work.");
  if(adminHash.length()==0){
    String initial="Vortex-"+String((uint32_t)ESP.getEfuseMac(),HEX).substring(2);
    adminHash=hashText(initial); prefs.putString("admin",adminHash);
    Serial.println("[VORTEX] First-run Admin username: dezavious");
    Serial.println("[VORTEX] First-run Admin password: "+initial);
  }
  if(directorHash.length()==0) prefs.putString("director",hashText("director"));
  if(WiFi.getMode()!=WIFI_AP) WiFi.mode(WIFI_AP);
  WiFi.softAP("Vortex-AI","VortexAI123");
  Serial.print("[VORTEX] Web UI: http://"); Serial.println(WiFi.softAPIP());
}

static void handleLogin(){
  String role=server.arg("role"), user=server.arg("username"), pass=server.arg("password");
  if(role=="admin" && user.equalsIgnoreCase("dezavious") && hashText(pass)==adminHash){sessionRole="admin";sessionUser="Dezavious Ojelade";}
  else if(role=="director" && user.equalsIgnoreCase("director") && hashText(pass)==directorHash){sessionRole="director";sessionUser=directorName;}
  else {sendJson(401,"{\"error\":\"Invalid login\"}");return;}
  sessionToken=randomToken(); sessionUntil=millis()+86400000UL;
  String n=sessionUser.length()?sessionUser:(sessionRole=="director"?"":"Dezavious Ojelade");
  sendJson(200,"{\"token\":\""+sessionToken+"\",\"role\":\""+sessionRole+"\",\"name\":\""+jsonEscape(n)+"\",\"purpose\":\""+jsonEscape(purposeText)+"\"}");
}

static void handleGuest(){sessionRole="guest";sessionUser=server.arg("name");if(sessionUser.length()==0)sessionUser="Guest User";sessionToken=randomToken();sessionUntil=millis()+43200000UL;sendJson(200,"{\"token\":\""+sessionToken+"\",\"role\":\"guest\",\"name\":\""+jsonEscape(sessionUser)+"\",\"purpose\":\""+jsonEscape(purposeText)+"\"}");}

static void handleChat(){
  if(!authed()){sendJson(401,"{\"error\":\"Session expired\"}");return;}
  String q=server.arg("message");
  if(q.length()==0){sendJson(400,"{\"error\":\"Empty message\"}");return;}
  String lower=q;lower.toLowerCase();
  if(lower=="208682de" && sessionRole=="admin"){
    sendJson(200,"{\"answer\":\"Admin purpose-edit mode is available in the Control Center.\"}");return;
  }
  String a=llm.generate(q,64);
  if(a.length()==0)a="I'm Vortex. I received your message, but my local model did not return text.";
  sendJson(200,"{\"answer\":\""+jsonEscape(a)+"\"}");
}

static void handlePurpose(){
  if(!authed("admin")){sendJson(403,"{\"error\":\"Admin only\"}");return;}
  purposeText=server.arg("purpose"); if(purposeText.length()==0) {sendJson(400,"{\"error\":\"Purpose cannot be empty\"}");return;}
  prefs.putString("purpose",purposeText);
  sendJson(200,"{\"message\":\"Vortex purpose updated.\"}");
}

static void handleCommand(){
  if(!authed("admin")){sendJson(403,"{\"error\":\"Admin only\"}");return;}
  String c=server.arg("command"); c.toLowerCase();
  if(c=="status") sendJson(200,"{\"result\":\"Vortex online. Role system active. AP IP: "+WiFi.softAPIP().toString()+"\"}");
  else if(c=="reboot"){sendJson(200,"{\"result\":\"Rebooting Vortex...\"}");delay(250);ESP.restart();}
  else if(c=="purpose") sendJson(200,"{\"result\":\""+jsonEscape(purposeText)+"\"}");
  else sendJson(403,"{\"error\":\"Unknown command. Add only safe registered commands through the command manager.\"}");
}

static void handleDirectorSetup(){
  if(!authed("director")){sendJson(403,"{\"error\":\"Director login required\"}");return;}
  String n=server.arg("name"); if(n.length()<2){sendJson(400,"{\"error\":\"Enter a valid name\"}");return;}
  directorName=n;prefs.putString("dname",n);sessionUser=n;sendJson(200,"{\"message\":\"Director name saved.\"}");
}

void vortexWebBegin(){
  loadConfig();
  const char *hdr[]={"X-Vortex-Session"}; server.collectHeaders(hdr,1);
  server.on("/",HTTP_GET,handleRoot);
  server.on("/api/login",HTTP_POST,handleLogin);
  server.on("/api/guest",HTTP_POST,handleGuest);
  server.on("/api/chat",HTTP_POST,handleChat);
  server.on("/api/admin/purpose",HTTP_POST,handlePurpose);
  server.on("/api/admin/command",HTTP_POST,handleCommand);
  server.on("/api/director/setup",HTTP_POST,handleDirectorSetup);
  server.onNotFound([](){server.send(404,"text/plain","Vortex endpoint not found");});
  server.begin();
}
void vortexWebLoop(){server.handleClient();}
