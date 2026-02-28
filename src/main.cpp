#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// --- HARDWARE PINS ---
const int PIN_UP = 4;     
const int PIN_DOWN = 18;  
const int PIN_SELECT = 19; 

// --- DUAL CONTROL ---
bool v_up = false, v_down = false, v_sel = false;
AsyncWebServer server(80);
bool serverStarted = false;

// --- SYSTEM STATES ---
enum AppState { HOME, MAIN_MENU, WIFI_MENU, SCANNING, SCAN_WIFI, ENTER_PASS, CONNECTING, BLUETOOTH_MENU, IR_MENU, WIFI_STATUS, SLEEP_MENU, AP_MODE };
AppState currentState = HOME;

// --- GLOBAL VARIABLES ---
bool wifiPower = false;
bool btPower = false;
int menuIdx = 0;
int scannedCount = 0;
int wifiListIdx = 0;
String selectedSSID = "";
String password = "";
bool v_submit = false;

// --- KEYBOARD CONTEXT ---
String* targetString = nullptr; 
AppState returnState = HOME;    
String keyboardLabel = "";      

const char* sets[] = {" ABCDEFGHIJKLMNOPQRSTUVWXYZ<X", " abcdefghijklmnopqrstuvwxyz<X", " 0123456789!@#$%^&*()_+-=<X"};
int currentSet = 0;
int charIdx = 0;

// --- PROTOTYPES ---
void drawStatusBar();
void drawHomeScreen();
void drawMainMenu();
void drawWiFiMenu();
void handleWiFiScan();
void drawWiFiList();
void drawKeyboard();
void handleConnection();
void drawPlaceholder(const char* title);
void drawWiFiStatus();
void drawSleepMenu();
void drawAPMode();

void setup() {
  Serial.begin(115200);
  pinMode(PIN_UP, INPUT); pinMode(PIN_DOWN, INPUT); pinMode(PIN_SELECT, INPUT);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) for(;;);
  WiFi.mode(WIFI_OFF); 
  display.setTextColor(WHITE);
}

void loop() {
  display.clearDisplay();
  drawStatusBar(); 

  switch (currentState) {
    case HOME:           drawHomeScreen();   break;
    case MAIN_MENU:      drawMainMenu();     break;
    case WIFI_MENU:      drawWiFiMenu();     break;
    case SCANNING:       handleWiFiScan();   break;
    case SCAN_WIFI:      drawWiFiList();     break;
    case ENTER_PASS:     drawKeyboard();     break;
    case CONNECTING:     handleConnection(); break;
    case WIFI_STATUS:    drawWiFiStatus();   break;
    case BLUETOOTH_MENU: drawPlaceholder("BLUETOOTH"); break;
    case IR_MENU:        drawPlaceholder("IR REMOTE"); break;
    case SLEEP_MENU:     drawSleepMenu();    break;
    case AP_MODE:        drawAPMode();       break;
  }

  display.display();
  delay(20); // Small loop delay for system stability
}

// --- THE IMPROVED WEB INTERFACE (HTML) ---
static const char index_html[] PROGMEM = R"rawliteral(
  <!DOCTYPE HTML><html><head><title>ESP REMOTE</title><meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body{font-family:Arial;text-align:center;background:#111;color:#0f0;} 
    .btn{background:#222;color:#0f0;border:1px solid #0f0;padding:15px;width:30%;margin:5px;font-weight:bold;border-radius:10px;}
    .sub-btn{background:#040;color:#fff;border:1px solid #0f0;padding:15px;width:65%;margin:5px;font-weight:bold;border-radius:10px;}
    .btn:active, .sub-btn:active{background:#0f0;color:#000;}
    #txt{padding:12px;width:80%;margin-top:10px;background:#000;color:#0f0;border:1px solid #0f0;font-size:18px;}
    #oledMirror{border:2px solid #555; background:#000; width:90%; max-width:300px; image-rendering: pixelated; margin-bottom:10px;}
  </style>
  </head><body>
    <h2>VIRTUAL OLED</h2>
    <canvas id="oledMirror" width="128" height="64"></canvas><br>
    <button class="btn" onclick="s('up')">UP</button><br>
    <button class="btn" onclick="s('sel')">SELECT</button><br>
    <button class="btn" onclick="s('down')">DOWN</button><br>
    <button class="sub-btn" onclick="f('/submit')">SUBMIT / SAVE</button>
    <hr>
    <input type="text" id="txt" placeholder="Inject Text..."><br>
    <button class="sub-btn" onclick="st()">SEND TO ESP</button>
    <script>
  const canvas = document.getElementById('oledMirror');
  const ctx = canvas.getContext('2d');

  // --- THE ESSENTIAL ENGINES ---
  function s(c){ fetch('/ctrl?c=' + c); } 
  function f(u){ fetch(u); }
  function st(){ 
    const val = document.getElementById('txt').value;
    fetch('/text?v=' + encodeURIComponent(val)); 
    document.getElementById('txt').value = ''; 
  }

  // --- THE MIRROR ENGINE (Fixed for SSD1306 Vertical Mapping) ---
  function updateMirror() {
    fetch('/screen').then(r => r.text()).then(hex => {
      ctx.fillStyle = "black"; 
      ctx.fillRect(0, 0, 128, 64);
      ctx.fillStyle = "#0af"; // Matches Blue OLED color
      
      for (let page = 0; page < 8; page++) {
        for (let x = 0; x < 128; x++) {
          let byteIdx = (page * 128) + x;
          let byte = parseInt(hex.substr(byteIdx * 2, 2), 16);
          for (let bit = 0; bit < 8; bit++) {
            if ((byte >> bit) & 0x01) {
              ctx.fillRect(x, (page * 8) + bit, 1, 1);
            }
          }
        }
      }
    }).catch(err => console.log("Mirror paused..."));
  }

  // Poll the screen every 400ms
  setInterval(updateMirror, 400);
</script>
  </body></html>)rawliteral";

void drawAPMode() {
  // --- START LOGIC (Only runs once) ---
  if (!serverStarted) {
    WiFi.softAP("SWISS_ARMY_ESP", NULL);
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *r){ r->send(200, "text/html", index_html); });
    server.on("/ctrl", HTTP_GET, [](AsyncWebServerRequest *r){ 
      String c = r->getParam("c")->value();
      if(c=="up") v_up=true; if(c=="down") v_down=true; if(c=="sel") v_sel=true;
      r->send(200); 
    });
    server.on("/text", HTTP_GET, [](AsyncWebServerRequest *r){
      if(r->hasParam("v") && targetString) *targetString = r->getParam("v")->value();
      r->send(200);
    });
    // 2. Add this route to your setupWebServer() or inside drawAPMode()
    server.on("/submit", HTTP_GET, [](AsyncWebServerRequest *r){ 
      v_submit = true; 
      r->send(200); 
    });
    // Remote Screen Data Route
    server.on("/screen", HTTP_GET, [](AsyncWebServerRequest *r){
      uint8_t* buf = display.getBuffer();
      String hex = "";
      for (int i = 0; i < 1024; i++) {
        if (buf[i] < 16) hex += "0";
        hex += String(buf[i], HEX);
      }
      r->send(200, "text/plain", hex);
    });
    server.begin();
    serverStarted = true;
    wifiPower = true;
  }

  // --- DISPLAY LOGIC ---
  display.setCursor(0, 15); display.println("--- REMOTE ACTIVE ---");
  display.setCursor(0, 28); display.println("SSID: SWISS_ARMY");
  display.println("IP  : 192.168.4.1");
  display.print("Clients: "); display.println(WiFi.softAPgetStationNum());
  
  display.drawFastHLine(0, 48, 128, WHITE);
  display.setCursor(0, 52); display.println("UP: BACK (Keep AP)");
  display.setCursor(0, 60); display.println("DN: STOP AP & EXIT");

  // --- OPTION 1: BACK (Keep AP Running) ---
  if (digitalRead(PIN_UP) == HIGH || v_up) {
    v_up = false;
    currentState = WIFI_MENU; // Exit screen but don't stop server
    while(digitalRead(PIN_UP) == HIGH); delay(200);
  }

  // --- OPTION 2: STOP (Kill AP Manually) ---
  if (digitalRead(PIN_DOWN) == HIGH || v_down) { 
    v_down = false; 
    WiFi.softAPdisconnect(true); 
    server.end(); 
    serverStarted = false; 
    wifiPower = false;
    currentState = WIFI_MENU; 
    while(digitalRead(PIN_DOWN) == HIGH); delay(300); 
  }
  
  // SELECT serves as a "Refresh/Stay" or you can map it to something else
  if (v_sel) v_sel = false; 
}
void drawStatusBar() {
  // --- POSITION 1: WIFI STATION (STA) - Far Right (X=115) ---
  if (wifiPower) {
    if (WiFi.status() == WL_CONNECTED) {
      display.fillRect(115, 1, 8, 8, WHITE); // Solid box = Connected to Router
    } else {
      display.drawRect(115, 1, 8, 8, WHITE); // Hollow box = Power ON but not connected
    }
  }

  // --- POSITION 2: ACCESS POINT (AP) - Left of STA (X=102) ---
  // Check if AP mode is physically active in the hardware
  if (WiFi.getMode() & WIFI_AP) { 
    display.setCursor(102, 1);
    display.print("A"); // "A" indicates the Phone Remote/AP is alive
    
    // Optional: Make the "A" blink if someone is actually connected to you
    if (WiFi.softAPgetStationNum() > 0) {
       if ((millis() / 500) % 2 == 0) display.drawPixel(108, 1, WHITE); 
    }
  }

  // --- POSITION 3: BLUETOOTH (If needed, X=90) ---
  if (btPower) { 
    display.setCursor(90, 1); 
    display.print("B"); 
  }

  display.drawFastHLine(0, 11, 128, WHITE);
}

void drawHomeScreen() {
  display.setTextSize(1);
  
  // --- ZONE 1: STATUS BAR PROTECTOR ---
  // (Y=0 to Y=12 is handled by drawStatusBar)
  
  // --- ZONE 2: CHIP DATA (Y=16) ---
  display.setCursor(0, 16);
  display.print("CPU:"); display.print(ESP.getCpuFreqMHz()); display.print("M");
  display.setCursor(65, 16);
  display.print("REV:"); display.print(ESP.getChipRevision());

  display.drawFastHLine(0, 26, 128, WHITE);

  // --- ZONE 3: MEMORY TELEMETRY (Y=30) ---
  uint32_t freeHeap = ESP.getFreeHeap();
  uint32_t minHeap = ESP.getMinFreeHeap();
  
  display.setCursor(0, 30);
  display.print("HEAP:"); display.print(freeHeap / 1024); display.print("K");
  
  int usedPct = 100 - ((freeHeap * 100) / ESP.getHeapSize());
  display.setCursor(65, 30);
  display.print("USE:"); display.print(usedPct); display.print("%");

  display.setCursor(0, 40);
  display.print("MIN :"); display.print(minHeap / 1024); display.print("K");
  display.setCursor(65, 40);
  display.print("FLS:"); display.print(ESP.getFlashChipSize() / 1024 / 1024); display.print("M");

  display.drawFastHLine(0, 50, 128, WHITE);

  // --- ZONE 4: SYSTEM & UPTIME (Y=54) ---
  display.setCursor(0, 54);
  long t = millis() / 1000;
  int h = t / 3600;
  int m = (t % 3600) / 60;
  int s = t % 60;
  
  // Professional formatted uptime
  display.printf("%02d:%02d:%02d", h, m, s);

  // Heartbeat Icon Fix (Uses ASCII code 3 for a real heart symbol)
  if((millis() / 500) % 2 == 0) {
    display.setCursor(110, 54); // Moved to the far right of the uptime row
    display.write(3); 
  }

  // --- DUAL CONTROL: SELECT (Physical + Virtual) ---
  if (digitalRead(PIN_SELECT) == HIGH || v_sel) { 
    v_sel = false; // Reset virtual flag
    currentState = MAIN_MENU; 
    while(digitalRead(PIN_SELECT) == HIGH); 
    delay(300); 
  }
}

void drawMainMenu() {
  const char* options[] = {"1. WiFi", "2. Bluetooth", "3. IR", "4. Sleep", "5. Exit"};
  for (int i = 0; i < 5; i++) {
    int y = 15 + (i * 10);
    if (i == menuIdx) { display.fillRect(0, y-1, 128, 10, WHITE); display.setTextColor(BLACK); }
    else display.setTextColor(WHITE);
    display.setCursor(5, y); display.println(options[i]);
  }
  display.setTextColor(WHITE);

  // --- DUAL CONTROL: DOWN ---
  if (digitalRead(PIN_DOWN) == HIGH || v_down) { 
    v_down = false; // Reset virtual flag immediately
    menuIdx = (menuIdx + 1) % 5; 
    while(digitalRead(PIN_DOWN) == HIGH); 
    delay(250); 
  }

  // --- DUAL CONTROL: UP ---
  if (digitalRead(PIN_UP) == HIGH || v_up) { 
    v_up = false; // Reset virtual flag immediately
    menuIdx = (menuIdx - 1 + 5) % 5; 
    while(digitalRead(PIN_UP) == HIGH); 
    delay(250); 
  }

  // --- DUAL CONTROL: SELECT ---
  if (digitalRead(PIN_SELECT) == HIGH || v_sel) {
    v_sel = false; // Reset virtual flag immediately
    if (menuIdx == 0) currentState = WIFI_MENU;
    if (menuIdx == 1) currentState = BLUETOOTH_MENU;
    if (menuIdx == 2) currentState = IR_MENU;
    if (menuIdx == 3) currentState = SLEEP_MENU;
    if (menuIdx == 4) currentState = HOME;
    
    while(digitalRead(PIN_SELECT) == HIGH);
    menuIdx = 0; 
    delay(300);
  }
}

void drawWiFiMenu() {
  display.setCursor(0, 15); 
  display.println("   --- WIFI ---");

  // All 6 options are here
  const char* options[] = { 
    wifiPower ? "Power: ON" : "Power: OFF", 
    "Start Scanning", 
    "Access Point (Remote)", 
    "Connection Status", 
    "Back"
  };
  int totalOpts = 5;

  // --- SCROLL LOGIC ---
  // This calculates which 4 items to show based on where the menuIdx is
  int startIdx = 0;
  if (menuIdx >= 4) {
    startIdx = menuIdx - 3; 
  }

  for (int i = 0; i < 4; i++) {
    int currentItem = startIdx + i;
    if (currentItem >= totalOpts) break;

    int y = 25 + (i * 10); // Comfortable 10-pixel spacing
    
    if (currentItem == menuIdx) {
      display.fillRect(0, y - 1, 128, 10, WHITE);
      display.setTextColor(BLACK);
    } else {
      display.setTextColor(WHITE);
    }

    display.setCursor(5, y);
    display.println(options[currentItem]);
  }
  display.setTextColor(WHITE);

  // --- SCROLL INDICATOR (Tiny dots on the right) ---
  if (totalOpts > 4) {
    display.drawFastVLine(126, 25, 38, WHITE);
    int indicatorY = 25 + (menuIdx * 30 / (totalOpts - 1));
    display.fillRect(125, indicatorY, 3, 5, WHITE);
  }

  // --- DUAL CONTROL: DOWN ---
  if (digitalRead(PIN_DOWN) == HIGH || v_down) { 
    v_down = false; 
    menuIdx = (menuIdx + 1) % totalOpts; 
    while(digitalRead(PIN_DOWN) == HIGH); 
    delay(200); 
  }
  
  // --- DUAL CONTROL: UP ---
  if (digitalRead(PIN_UP) == HIGH || v_up) { 
    v_up = false; 
    menuIdx = (menuIdx - 1 + totalOpts) % totalOpts; 
    while(digitalRead(PIN_UP) == HIGH); 
    delay(200); 
  }

  // --- DUAL CONTROL: SELECT ---
  if (digitalRead(PIN_SELECT) == HIGH || v_sel) {
    v_sel = false;
    if (menuIdx == 0) { 
      wifiPower = !wifiPower; 
      if(wifiPower) WiFi.mode(WIFI_STA); else WiFi.mode(WIFI_OFF); 
    }
    else if (menuIdx == 1 && wifiPower) currentState = SCANNING;
    else if (menuIdx == 2) currentState = AP_MODE;     // Access Point / Remote Mode
    else if (menuIdx == 3) currentState = WIFI_STATUS; // Connection Status
    else if (menuIdx == 4) currentState = MAIN_MENU;   // Back

    while(digitalRead(PIN_SELECT) == HIGH);
    menuIdx = 0; 
    delay(300);
  }
}

void handleWiFiScan() {
  display.setCursor(30, 35); display.print("Scanning..."); display.display();
  scannedCount = WiFi.scanNetworks();
  currentState = (scannedCount > 0) ? SCAN_WIFI : WIFI_MENU;
}

void drawWiFiList() {
  display.setCursor(0, 15); display.println("   PICK NETWORK:");
  
  int totalOptions = scannedCount + 1;
  int startIdx = (wifiListIdx >= 4) ? wifiListIdx - 3 : 0;
  
  for (int i = 0; i < 4; i++) {
    int cur = startIdx + i; 
    if (cur >= totalOptions) break;

    int y = 25 + (i * 10);
    if (cur == wifiListIdx) { 
      display.fillRect(0, y-1, 128, 9, WHITE); 
      display.setTextColor(BLACK); 
    } else { 
      display.setTextColor(WHITE); 
    }
    
    display.setCursor(5, y);
    if (cur == 0) {
      display.print("<-- BACK");
    } else {
      display.print(WiFi.SSID(cur - 1).substring(0, 15));
    }
  }
  display.setTextColor(WHITE);

  // --- DUAL CONTROL: DOWN ---
  if (digitalRead(PIN_DOWN) == HIGH || v_down) { 
    v_down = false; // Reset virtual flag
    wifiListIdx = (wifiListIdx + 1) % totalOptions; 
    while(digitalRead(PIN_DOWN) == HIGH); 
    delay(200); 
  }

  // --- DUAL CONTROL: UP ---
  if (digitalRead(PIN_UP) == HIGH || v_up) { 
    v_up = false; // Reset virtual flag
    wifiListIdx = (wifiListIdx - 1 + totalOptions) % totalOptions; 
    while(digitalRead(PIN_UP) == HIGH); 
    delay(200); 
  }
  
  // --- DUAL CONTROL: SELECT ---
  if (digitalRead(PIN_SELECT) == HIGH || v_sel) { 
    v_sel = false; // Reset virtual flag
    if (wifiListIdx == 0) {
      currentState = WIFI_MENU;
    } else {
      selectedSSID = WiFi.SSID(wifiListIdx - 1); 
      targetString = &password; 
      keyboardLabel = "WIFI PASS"; 
      returnState = CONNECTING;
      currentState = ENTER_PASS; 
    }
    while(digitalRead(PIN_SELECT) == HIGH); 
    delay(400); 
  }
}
void drawWiFiStatus() {
  display.setCursor(0, 15);
  display.println("--- CONN. STATUS ---");
  
  // --- CASE 1: Connected to a Router (STA Mode) ---
  if(WiFi.status() == WL_CONNECTED) {
    display.setCursor(0, 28);
    display.print("SSID: "); display.println(WiFi.SSID());
    display.print("IP  : "); display.println(WiFi.localIP().toString());
    display.print("RSSI: "); display.print(WiFi.RSSI()); display.println(" dBm");
  } 
  // --- CASE 2: Running as an Access Point (AP Mode) ---
  else if (WiFi.getMode() & WIFI_AP) {
    display.setCursor(0, 28);
    display.print("AP SSID: "); display.println("SWISS_ARMY");
    display.print("AP IP  : "); display.println(WiFi.softAPIP().toString());
    display.print("Clients: "); display.println(WiFi.softAPgetStationNum());
  } 
  // --- CASE 3: No Active Connection ---
  else {
    display.setCursor(0, 35);
    display.println("Status: IDLE");
    display.println("Radio: OFF");
  }

  display.setCursor(20, 56);
  display.print("[SELECT: BACK]");
  
  // --- DUAL CONTROL: SELECT (Physical + Virtual) ---
  if (digitalRead(PIN_SELECT) == HIGH || v_sel) {
    v_sel = false; // Reset virtual flag
    currentState = WIFI_MENU;
    while(digitalRead(PIN_SELECT) == HIGH);
    delay(300);
  }
}
void drawKeyboard() {
  if (targetString == nullptr) { currentState = MAIN_MENU; return; }
  const char* activeSet = sets[currentSet];
  int len = strlen(activeSet);
  
  display.setTextSize(1);
  display.setCursor(0, 13); display.print(keyboardLabel);
  
  // This line is where the magic happens: as you type on your phone, 
  // the text route updates *targetString, and it appears here instantly.
  display.setCursor(0, 23); display.print("VAL: "); display.print(*targetString); display.println("_");
  display.drawFastHLine(0, 31, 128, WHITE);

  display.setCursor(15, 45); display.print(activeSet[(charIdx - 2 + len) % len]);
  display.setCursor(40, 45); display.print(activeSet[(charIdx - 1 + len) % len]);
  display.setTextSize(2); display.setCursor(58, 41); display.print(activeSet[charIdx]);
  display.drawRoundRect(54, 38, 22, 22, 3, WHITE);
  display.setTextSize(1);
  display.setCursor(88, 45); display.print(activeSet[(charIdx + 1) % len]);
  display.setCursor(113, 45); display.print(activeSet[(charIdx + 2) % len]);

  // --- DUAL CONTROL: DOWN ---
  if (digitalRead(PIN_DOWN) == HIGH || v_down) { 
    v_down = false; 
    charIdx = (charIdx + 1) % len; 
    while(digitalRead(PIN_DOWN) == HIGH); 
    delay(200); 
  }

  // --- DUAL CONTROL: UP ---
  if (digitalRead(PIN_UP) == HIGH || v_up) {
    v_up = false;
    unsigned long upStart = millis();
    // Physical button long-press for set change
    while(digitalRead(PIN_UP) == HIGH) {
      if(millis() - upStart > 1000) { 
        currentSet = (currentSet + 1) % 3; charIdx = 0; 
        while(digitalRead(PIN_UP) == HIGH); 
        delay(400); 
        break; 
      }
    }
    // Short press or Virtual UP
    if(millis() - upStart <= 1000) {
      charIdx = (charIdx - 1 + len) % len;
      delay(200);
    }
  }

  // --- DUAL CONTROL: SELECT ---
  if (digitalRead(PIN_SELECT) == HIGH || v_sel || v_submit) {
    unsigned long start = millis();
    bool isRemoteSubmit = v_submit; // Store the flag state
    v_submit = false; // Reset immediately
    
    if (digitalRead(PIN_SELECT) == HIGH) {
      while(digitalRead(PIN_SELECT) == HIGH); 
    }
    
    unsigned long dur = (v_sel || isRemoteSubmit) ? 0 : (millis() - start);
    v_sel = false;

    // Logic: If it's a long press OR the dedicated Remote Submit button
    if (dur > 2500 || isRemoteSubmit) { 
      currentState = returnState; // Success! Save and move to connection
      delay(500); 
    }
    else if (dur > 800) { // Medium press: Backspace
      if ((*targetString).length() > 0) (*targetString).remove((*targetString).length() - 1); 
      delay(300); 
    }
    else { // Short press (Physical or Virtual Select)
      char selectedChar = activeSet[charIdx];
      if (selectedChar == '<') { 
        if ((*targetString).length() > 0) (*targetString).remove((*targetString).length() - 1); 
      } 
      else if (selectedChar == 'X') { 
        currentState = WIFI_MENU; 
      }
      else {
        *targetString += selectedChar;
      }
      delay(300); 
    }
  }
}

void handleConnection() {
  display.clearDisplay();
  display.setCursor(20, 30);
  display.print("Connecting...");
  display.setCursor(20, 42);
  display.print(selectedSSID.substring(0, 15));
  display.display();

  // --- STEP 1: PREPARE RADIO ---
  // If AP is running, we keep it alive but ensure we are in STA+AP mode
  if (WiFi.getMode() & WIFI_AP) {
    WiFi.mode(WIFI_AP_STA); 
  } else {
    WiFi.mode(WIFI_STA);
  }

  // --- STEP 2: BEGIN HANDSHAKE ---
  WiFi.begin(selectedSSID.c_str(), password.c_str());
  
  int attempts = 0;
  // We check for 20 attempts (10 seconds total)
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    attempts++;
    
    // Visual progress bar
    display.fillRect(20, 52, attempts * 4, 3, WHITE);
    display.display();
  }
  
  // --- STEP 3: RESULT HANDLING ---
  display.clearDisplay();
  display.setCursor(20, 30);
  
  if (WiFi.status() == WL_CONNECTED) {
    display.println("CONNECTED!");
    display.setCursor(20, 42);
    display.println(WiFi.localIP().toString());
  } else {
    display.println("FAILED");
    display.setCursor(10, 42);
    display.println("Check Password");
    WiFi.disconnect(); // Clean up failed attempt
  }
  
  display.display();
  delay(2000); 
  currentState = HOME;
}

void drawPlaceholder(const char* title) {
  display.setCursor(30, 30); display.print(title);
  display.setCursor(20, 45); display.print("Locked ");
  if (digitalRead(PIN_SELECT) == HIGH) { currentState = MAIN_MENU; while(digitalRead(PIN_SELECT) == HIGH); delay(300); }
}

void drawSleepMenu() {
  display.setCursor(0, 10); display.println("   --- SLEEP ---");
  const char* options[] = {"1. Light Sleep", "2. Mod. Sleep", "3. Deep Sleep", "4. Back"};
  for (int i = 0; i < 4; i++) {
    int y = 22 + (i * 10);
    if (i == menuIdx) { display.fillRect(0, y-1, 128, 10, WHITE); display.setTextColor(BLACK); }
    else display.setTextColor(WHITE);
    display.setCursor(5, y); display.println(options[i]);
  }
  display.setTextColor(WHITE);

  if (digitalRead(PIN_DOWN) == HIGH) { menuIdx = (menuIdx + 1) % 4; while(digitalRead(PIN_DOWN) == HIGH); delay(250); }
  if (digitalRead(PIN_UP) == HIGH)   { menuIdx = (menuIdx - 1 + 4) % 4; while(digitalRead(PIN_UP) == HIGH); delay(250); }
  if (digitalRead(PIN_SELECT) == HIGH) {
    if (menuIdx == 0) {
      display.clearDisplay(); display.setCursor(30, 30); display.print("Light Sleep"); display.display();
      delay(1000);
      gpio_wakeup_enable((gpio_num_t)PIN_SELECT, GPIO_INTR_HIGH_LEVEL);
      esp_sleep_enable_gpio_wakeup();
      esp_light_sleep_start();
      Wire.begin(); // Re-initialize I2C in case it was powered down
      currentState = HOME;
    }
    else if (menuIdx == 1) {
      display.clearDisplay(); display.setCursor(20, 30); display.print("Modem Sleep"); display.display();
      delay(1000);
      
      // -- MODEM SLEEP CONFIGURATION --
      WiFi.disconnect();
      WiFi.mode(WIFI_OFF);
      btStop(); // Shuts down Bluetooth
      wifiPower = false;
      btPower = false;

      display.ssd1306_command(SSD1306_DISPLAYOFF);
      while(digitalRead(PIN_SELECT) == LOW) { delay(100); }
      display.ssd1306_command(SSD1306_DISPLAYON);
      currentState = HOME;
    }
    else if (menuIdx == 2) {
      display.clearDisplay(); display.setCursor(30, 30); display.print("Deep Sleep"); 
      display.setCursor(15, 45); display.print("Press UP to Wake"); display.display();
      delay(2000);
      esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_UP, 1); // PIN_UP is an RTC GPIO
      esp_deep_sleep_start();
    }
    else if (menuIdx == 3) {
      currentState = MAIN_MENU;
    }
    
    while(digitalRead(PIN_SELECT) == HIGH);
    menuIdx = 0; delay(300);
  }
}
