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

// --- SYSTEM STATES ---
enum AppState { HOME, MAIN_MENU, WIFI_MENU, SCANNING, SCAN_WIFI, ENTER_PASS, CONNECTING, BLUETOOTH_MENU, IR_MENU };
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

const char* sets[] = {" ABCDEFGHIJKLMNOPQRSTUVWXYZ<", " abcdefghijklmnopqrstuvwxyz<", " 0123456789!@#$%^&*()_+-=<"};
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
    case BLUETOOTH_MENU: drawPlaceholder("BLUETOOTH"); break;
    case IR_MENU:        drawPlaceholder("IR REMOTE"); break;
  }

  display.display();
  delay(20); // Small loop delay for system stability
}

// ==========================================
// STABILIZED INPUT LOGIC (The "Cooldown" Fix)
// ==========================================

void drawStatusBar() {
  if (wifiPower) {
    if (WiFi.status() == WL_CONNECTED) display.fillRect(115, 1, 8, 8, WHITE); 
    else display.drawRect(115, 1, 8, 8, WHITE); 
  }
  if (btPower) { display.setCursor(102, 1); display.print("B"); }
  display.drawFastHLine(0, 11, 128, WHITE);
}

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
    while(digitalRead(PIN_SELECT) == HIGH); // Wait for release
    delay(300); // COOLDOWN
  }
}

void drawMainMenu() {
  const char* options[] = {"1. WiFi Config", "2. Bluetooth", "3. IR Remote", "4. Exit"};
  for (int i = 0; i < 4; i++) {
    int y = 18 + (i * 11);
    if (i == menuIdx) { display.fillRect(0, y-1, 128, 10, WHITE); display.setTextColor(BLACK); }
    else display.setTextColor(WHITE);
    display.setCursor(5, y); display.println(options[i]);
  }
  display.setTextColor(WHITE);

  if (digitalRead(PIN_DOWN) == HIGH) { 
    menuIdx = (menuIdx + 1) % 4; 
    while(digitalRead(PIN_DOWN) == HIGH); 
    delay(250); 
  }
  if (digitalRead(PIN_UP) == HIGH) { 
    menuIdx = (menuIdx - 1 + 4) % 4; 
    while(digitalRead(PIN_UP) == HIGH); 
    delay(250); 
  }
  if (digitalRead(PIN_SELECT) == HIGH) {
    if (menuIdx == 0) currentState = WIFI_MENU;
    if (menuIdx == 1) currentState = BLUETOOTH_MENU;
    if (menuIdx == 2) currentState = IR_MENU;
    if (menuIdx == 3) currentState = HOME;
    while(digitalRead(PIN_SELECT) == HIGH);
    delay(300);
  }
}

void drawWiFiMenu() {
  display.setCursor(0, 15); display.println("   --- WIFI ---");
  const char* options[] = { wifiPower ? "Power: ON" : "Power: OFF", "Start Scanning", "Back"};
  for (int i = 0; i < 3; i++) {
    int y = 28 + (i * 11);
    if (i == menuIdx) { display.fillRect(0, y-1, 128, 10, WHITE); display.setTextColor(BLACK); }
    else display.setTextColor(WHITE);
    display.setCursor(5, y); display.println(options[i]);
  }
  display.setTextColor(WHITE);

  if (digitalRead(PIN_DOWN) == HIGH) { menuIdx = (menuIdx + 1) % 3; while(digitalRead(PIN_DOWN) == HIGH); delay(250); }
  if (digitalRead(PIN_UP) == HIGH)   { menuIdx = (menuIdx - 1 + 3) % 3; while(digitalRead(PIN_UP) == HIGH); delay(250); }
  if (digitalRead(PIN_SELECT) == HIGH) {
    if (menuIdx == 0) { wifiPower = !wifiPower; if(wifiPower) WiFi.mode(WIFI_STA); else WiFi.mode(WIFI_OFF); }
    if (menuIdx == 1 && wifiPower) currentState = SCANNING;
    if (menuIdx == 2) currentState = MAIN_MENU;
    while(digitalRead(PIN_SELECT) == HIGH);
    delay(300);
  }
}

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
    if (cur == wifiListIdx) { display.fillRect(0, y-1, 128, 9, WHITE); display.setTextColor(BLACK); }
    else display.setTextColor(WHITE);
    display.setCursor(5, y); display.print(WiFi.SSID(cur).substring(0, 15));
  }
  display.setTextColor(WHITE);
  if (digitalRead(PIN_DOWN) == HIGH) { wifiListIdx = (wifiListIdx + 1) % scannedCount; while(digitalRead(PIN_DOWN) == HIGH); delay(250); }
  if (digitalRead(PIN_UP) == HIGH)   { wifiListIdx = (wifiListIdx - 1 + scannedCount) % scannedCount; while(digitalRead(PIN_UP) == HIGH); delay(250); }
  if (digitalRead(PIN_SELECT) == HIGH) { 
    selectedSSID = WiFi.SSID(wifiListIdx); 
    targetString = &password; keyboardLabel = "WIFI PASS"; returnState = CONNECTING;
    currentState = ENTER_PASS; 
    while(digitalRead(PIN_SELECT) == HIGH); 
    delay(400); 
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

    if (dur > 2500) { currentState = returnState; delay(500); }
    else if (dur > 800) { if ((*targetString).length() > 0) (*targetString).remove((*targetString).length() - 1); delay(300); }
    else {
      char selectedChar = activeSet[charIdx];
      if (selectedChar == '<') { if ((*targetString).length() > 0) (*targetString).remove((*targetString).length() - 1); }
      else *targetString += selectedChar;
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
  display.setCursor(20, 45); display.print("Module Locked");
  if (digitalRead(PIN_SELECT) == HIGH) { currentState = MAIN_MENU; while(digitalRead(PIN_SELECT) == HIGH); delay(300); }
}