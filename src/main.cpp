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

void drawAPMode() {
  static const char index_html[] PROGMEM = R"rawliteral(
  <!DOCTYPE HTML><html><head><title>REMOTE</title><meta name="viewport" content="width=device-width, initial-scale=1">
  <style>body{font-family:Arial;text-align:center;background:#111;color:#0f0;} .btn{background:#222;color:#0f0;border:1px solid #0f0;padding:20px;width:30%;margin:5px;font-weight:bold;border-radius:10px;} .btn:active{background:#0f0;color:#000;} #txt{padding:12px;width:80%;margin-top:20px;background:#000;color:#0f0;border:1px solid #0f0;font-size:18px;}</style>
  </head><body><h2>REMOTE CONSOLE</h2><button class="btn" onclick="s('up')">UP</button><br><button class="btn" onclick="s('sel')">SELECT</button><br><button class="btn" onclick="s('down')">DOWN</button>
  <br><input type="text" id="txt" placeholder="Inject Keyboard..."><br><button class="btn" style="width:60%" onclick="st()">SEND TEXT</button>
  <script>function s(c){fetch('/ctrl?c='+c);} function st(){fetch('/text?v='+encodeURIComponent(document.getElementById('txt').value)); document.getElementById('txt').value='';}</script>
  </body></html>)rawliteral";

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
  if (wifiPower) {
    wifi_mode_t mode = WiFi.getMode();
    if (mode == WIFI_AP || mode == WIFI_AP_STA) {
      // AP mode: draw broadcast/hotspot icon (concentric arcs + dot)
      display.fillCircle(119, 8, 1, WHITE);               // center dot
      display.drawCircle(119, 8, 3, WHITE);                // inner arc
      display.drawCircle(119, 8, 5, WHITE);                // outer arc
      // Mask bottom half to make it look like upward-radiating arcs
      display.fillRect(114, 9, 12, 4, BLACK);
    } else if (WiFi.status() == WL_CONNECTED) {
      display.fillRect(115, 1, 8, 8, WHITE);              // filled square
    } else {
      display.drawRect(115, 1, 8, 8, WHITE);              // empty square
    }
  }
  if (btPower) { display.setCursor(102, 1); display.print("B"); }
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
  display.setCursor(0, 23); display.print("VAL: "); display.print(*targetString); display.println("_");
  display.drawFastHLine(0, 31, 128, WHITE);

  display.setCursor(15, 45); display.print(activeSet[(charIdx - 2 + len) % len]);
  display.setCursor(40, 45); display.print(activeSet[(charIdx - 1 + len) % len]);
  display.setTextSize(2); display.setCursor(58, 41); display.print(activeSet[charIdx]);
  display.drawRoundRect(54, 38, 22, 22, 3, WHITE);
  display.setTextSize(1);
  display.setCursor(88, 45); display.print(activeSet[(charIdx + 1) % len]);
  display.setCursor(113, 45); display.print(activeSet[(charIdx + 2) % len]);

  if (digitalRead(PIN_DOWN) == HIGH) { 
    charIdx = (charIdx + 1) % len; 
    while(digitalRead(PIN_DOWN) == HIGH); 
    delay(200); // Prevents fast scrolling
  }
  if (digitalRead(PIN_UP) == HIGH) {
    unsigned long upStart = millis();
    while(digitalRead(PIN_UP) == HIGH) {
      if(millis() - upStart > 1000) { 
        currentSet = (currentSet + 1) % 3; charIdx = 0; 
        while(digitalRead(PIN_UP) == HIGH); 
        delay(400); 
        break; 
      }
    }
    if(millis() - upStart <= 1000) {
      charIdx = (charIdx - 1 + len) % len;
      delay(200);
    }
  }
if (digitalRead(PIN_SELECT) == HIGH) {
    unsigned long start = millis();
    while(digitalRead(PIN_SELECT) == HIGH); 
    unsigned long dur = millis() - start;

    if (dur > 2500) {
       currentState = returnState; // ENTER/SAVE
       delay(500); 
    }
    else if (dur > 800) { 
       if ((*targetString).length() > 0) (*targetString).remove((*targetString).length() - 1); 
       delay(300); 
    }
    else {
      char selectedChar = activeSet[charIdx];
      if (selectedChar == '<') { 
        if ((*targetString).length() > 0) (*targetString).remove((*targetString).length() - 1); 
      } 
      else if (selectedChar == 'X') { 
        // --- NEW CANCEL LOGIC ---
        currentState = WIFI_MENU; // Exit without connecting
      }
      else {
        *targetString += selectedChar;
      }
      delay(300); 
    }
  }
}

void handleConnection() {
  display.setCursor(20, 35); display.print("Connecting..."); display.display();
  WiFi.begin(selectedSSID.c_str(), password.c_str());
  int t = 0; while (WiFi.status() != WL_CONNECTED && t < 20) { delay(500); t++; }
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
