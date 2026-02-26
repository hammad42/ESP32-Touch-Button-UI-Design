#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// --- HARDWARE PINS ---
const int PIN_UP = 4;     
const int PIN_DOWN = 18;  
const int PIN_SELECT = 19; 

// --- SYSTEM STATES (Fixed Enum) ---
enum AppState { HOME, MAIN_MENU, WIFI_MENU, SCANNING, SCAN_WIFI, ENTER_PASS, CONNECTING, BLUETOOTH_MENU, IR_MENU };
AppState currentState = HOME;

// --- GLOBAL SYSTEM VARIABLES ---
bool wifiPower = false;
bool btPower = false;
int menuIdx = 0;
int scannedCount = 0;
int wifiListIdx = 0;
String selectedSSID = "";
String password = "";

// Keyboard Data
const char* sets[] = {" ABCDEFGHIJKLMNOPQRSTUVWXYZ<", " abcdefghijklmnopqrstuvwxyz<", " 0123456789!@#$%^&*()_+-=<"};
int currentSet = 0;
int charIdx = 0;

// --- FUNCTION PROTOTYPES ---
void drawStatusBar();
void drawHomeScreen();
void drawMainMenu();
void drawWiFiMenu();
void handleWiFiScan();
void drawWiFiList();
void drawKeyboard();
void handleConnection();
void drawPlaceholder(const char* title);

// ==========================================
// MAIN SETUP & LOOP
// ==========================================

void setup() {
  Serial.begin(115200);
  pinMode(PIN_UP, INPUT); pinMode(PIN_DOWN, INPUT); pinMode(PIN_SELECT, INPUT);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) for(;;);
  
  WiFi.mode(WIFI_OFF); 
  display.clearDisplay();
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
    case BLUETOOTH_MENU: drawPlaceholder("BLUETOOTH"); break;
    case IR_MENU:        drawPlaceholder("IR REMOTE"); break;
  }

  display.display();
  delay(10);
}

// ==========================================
// MODULE: STATUS BAR (Android Philosophy)
// ==========================================
void drawStatusBar() {
  // WiFi Icon Logic: Android-inspired
  if (wifiPower) {
    if (WiFi.status() == WL_CONNECTED) {
      display.fillRect(115, 1, 8, 8, WHITE); // Solid Square = Connected
    } else {
      display.drawRect(115, 1, 8, 8, WHITE); // Hollow Square = Power ON
    }
  }
  
  // Bluetooth Icon (Simplified)
  if (btPower) {
    display.setCursor(102, 1);
    display.print("B"); 
  }

  display.drawFastHLine(0, 11, 128, WHITE);
}

// ==========================================
// MODULE: HOME SCREEN
// ==========================================
void drawHomeScreen() {
  display.setTextSize(1);
  display.setCursor(0, 25);
  
  if(WiFi.status() == WL_CONNECTED) {
    display.println("IP: " + WiFi.localIP().toString());
    display.print("RSSI: "); display.print(WiFi.RSSI()); display.println(" dBm");
  } else {
    display.println("SYSTEM READY");
    display.print("WIFI: "); display.println(wifiPower ? "ON" : "OFF");
  }

  display.setCursor(20, 56);
  display.print("[TOUCH SEL FOR MENU]");
  
  if (digitalRead(PIN_SELECT) == HIGH) { 
    currentState = MAIN_MENU; 
    menuIdx = 0; 
    delay(300); 
  }
}

// ==========================================
// MODULE: MENUS
// ==========================================
void drawMainMenu() {
  const char* options[] = {"1. WiFi Config", "2. Bluetooth", "3. IR Remote", "4. Exit"};
  for (int i = 0; i < 4; i++) {
    int y = 18 + (i * 11);
    if (i == menuIdx) { 
      display.fillRect(0, y-1, 128, 10, WHITE); 
      display.setTextColor(BLACK); 
    } else { 
      display.setTextColor(WHITE); 
    }
    display.setCursor(5, y); display.println(options[i]);
  }
  display.setTextColor(WHITE);

  if (digitalRead(PIN_DOWN) == HIGH) { menuIdx = (menuIdx + 1) % 4; delay(200); }
  if (digitalRead(PIN_UP) == HIGH)   { menuIdx = (menuIdx - 1 + 4) % 4; delay(200); }
  if (digitalRead(PIN_SELECT) == HIGH) {
    if (menuIdx == 0) currentState = WIFI_MENU;
    if (menuIdx == 1) currentState = BLUETOOTH_MENU;
    if (menuIdx == 2) currentState = IR_MENU;
    if (menuIdx == 3) currentState = HOME;
    menuIdx = 0; delay(300);
  }
}

void drawWiFiMenu() {
  display.setCursor(0, 15);
  display.println("   --- WIFI ---");
  const char* options[] = { wifiPower ? "Power: ON" : "Power: OFF", "Start Scanning", "Back"};
  
  for (int i = 0; i < 3; i++) {
    int y = 28 + (i * 11);
    if (i == menuIdx) { 
      display.fillRect(0, y-1, 128, 10, WHITE); 
      display.setTextColor(BLACK); 
    } else { 
      display.setTextColor(WHITE); 
    }
    display.setCursor(5, y); display.println(options[i]);
  }
  display.setTextColor(WHITE);

  if (digitalRead(PIN_DOWN) == HIGH) { menuIdx = (menuIdx + 1) % 3; delay(200); }
  if (digitalRead(PIN_UP) == HIGH)   { menuIdx = (menuIdx - 1 + 3) % 3; delay(200); }
  
  if (digitalRead(PIN_SELECT) == HIGH) {
    if (menuIdx == 0) { 
      wifiPower = !wifiPower; 
      if(wifiPower) WiFi.mode(WIFI_STA); else WiFi.mode(WIFI_OFF);
    }
    if (menuIdx == 1 && wifiPower) currentState = SCANNING;
    if (menuIdx == 2) currentState = MAIN_MENU;
    delay(300);
  }
}

// ==========================================
// MODULE: WIFI SCAN & LIST
// ==========================================
void handleWiFiScan() {
  display.setCursor(30, 35); display.print("Scanning..."); display.display();
  scannedCount = WiFi.scanNetworks();
  currentState = (scannedCount > 0) ? SCAN_WIFI : WIFI_MENU;
}

void drawWiFiList() {
  display.setCursor(0, 15); display.println("  PICK NETWORK:");
  int startIdx = (wifiListIdx >= 4) ? wifiListIdx - 3 : 0;
  
  for (int i = 0; i < 4; i++) {
    int cur = startIdx + i; if (cur >= scannedCount) break;
    int y = 25 + (i * 10);
    if (cur == wifiListIdx) { 
      display.fillRect(0, y-1, 128, 9, WHITE); 
      display.setTextColor(BLACK); 
    } else { 
      display.setTextColor(WHITE); 
    }
    display.setCursor(5, y); display.print(WiFi.SSID(cur).substring(0, 15));
  }
  display.setTextColor(WHITE);
  
  if (digitalRead(PIN_DOWN) == HIGH) { wifiListIdx = (wifiListIdx + 1) % scannedCount; delay(200); }
  if (digitalRead(PIN_UP) == HIGH)   { wifiListIdx = (wifiListIdx - 1 + scannedCount) % scannedCount; delay(200); }
  if (digitalRead(PIN_SELECT) == HIGH) { 
    selectedSSID = WiFi.SSID(wifiListIdx); 
    currentState = ENTER_PASS; 
    delay(300); 
  }
}

// ==========================================
// MODULE: KEYBOARD
// ==========================================
void drawKeyboard() {
  const char* activeSet = sets[currentSet];
  int len = strlen(activeSet);
  
  // 1. Professional Header
  display.setTextSize(1);
  display.setCursor(0, 13);
  display.print("ID: "); display.print(selectedSSID.substring(0, 10));
  display.setCursor(70, 13);
  display.print("PW:"); display.print(password); display.println("_");
  display.drawFastHLine(0, 23, 128, WHITE);

  // 2. The Focus-Zoom Ribbon
  // Positions for 5 characters: Far Left, Near Left, CENTER, Near Right, Far Right
  
  // Far Left (-2)
  display.setTextSize(1);
  display.setCursor(15, 38);
  display.print(activeSet[(charIdx - 2 + len) % len]);

  // Near Left (-1)
  display.setCursor(40, 38);
  display.print(activeSet[(charIdx - 1 + len) % len]);

  // --- CENTER FOCUS (The Selected Character) ---
  display.setTextSize(2);           // Larger font for focus
  display.setCursor(58, 34);
  display.print(activeSet[charIdx]);
  display.drawRoundRect(54, 31, 22, 22, 3, WHITE); // Rounded selection box
  display.setTextSize(1);           // Reset to small
  // ----------------------------------------------

  // Near Right (+1)
  display.setCursor(88, 38);
  display.print(activeSet[(charIdx + 1) % len]);

  // Far Right (+2)
  display.setCursor(113, 38);
  display.print(activeSet[(charIdx + 2) % len]);

  // 3. Navigation Instruction Footer
  display.drawFastHLine(0, 56, 128, WHITE);
  display.setCursor(10, 58);
  display.print("HOLD UP:Shift  HOLD SEL:Connect");

  // --- INPUT HANDLING ---
  if (digitalRead(PIN_DOWN) == HIGH) { charIdx = (charIdx + 1) % len; delay(150); }
  if (digitalRead(PIN_UP) == HIGH) {
    unsigned long upStart = millis();
    while(digitalRead(PIN_UP) == HIGH);
    if(millis() - upStart > 1000) { // Shift shortcut
      currentSet = (currentSet + 1) % 3;
      charIdx = 0;
    } else {
      charIdx = (charIdx - 1 + len) % len;
    }
    delay(150);
  }
  
  if (digitalRead(PIN_SELECT) == HIGH) {
    unsigned long start = millis();
    while(digitalRead(PIN_SELECT) == HIGH);
    unsigned long dur = millis() - start;

    if (dur > 2500) {
       currentState = CONNECTING; 
    } else if (dur > 800) {
       if (password.length() > 0) password.remove(password.length() - 1);
    } else {
       password += activeSet[charIdx];
    }
  }
}

void handleConnection() {
  display.setCursor(20, 35); display.print("Connecting..."); display.display();
  WiFi.begin(selectedSSID.c_str(), password.c_str());
  int t = 0; while (WiFi.status() != WL_CONNECTED && t < 20) { delay(500); t++; }
  currentState = HOME;
}

// ==========================================
// MODULE: PLACEHOLDER
// ==========================================
void drawPlaceholder(const char* title) {
  display.setCursor(30, 30); display.print(title);
  display.setCursor(20, 45); display.print("Module Locked");
  if (digitalRead(PIN_SELECT) == HIGH) { currentState = MAIN_MENU; delay(300); }
}