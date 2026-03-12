#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include "globals.h"
#include "display_utils.h"
#include "wifi_logic.h"
#include "menu_logic.h"
#include "keyboard_logic.h"
#include "connection_logic.h"
#include "sleep_logic.h" // Add this at the top
#include "placeholders.h" // Add this at the top
#include "subghz_logic.h"
// --- INSTANTIATE GLOBALS ---
Adafruit_SSD1306 display(128, 64, &Wire, -1);
AsyncWebServer server(80);
AppState currentState = HOME;
AppState returnState = HOME;
volatile bool v_up = false, v_down = false, v_sel = false, v_submit = false;
bool wifiPower = false, btPower = false, serverStarted = false;
int menuIdx = 0, scannedCount = 0, wifiListIdx = 0;
String selectedSSID = "", password = "", keyboardLabel = "";
String* targetString = nullptr;
const char* sets[] = {" ABCDEFGHIJKLMNOPQRSTUVWXYZ<X", " abcdefghijklmnopqrstuvwxyz<X", " 0123456789!@#$%^&*()_+-=<X"};
int currentSet = 0, charIdx = 0;
bool subghzInit = false; // Track if CC1101 is physically present

void setup() {
  Serial.begin(115200);
  pinMode(PIN_UP, INPUT); pinMode(PIN_DOWN, INPUT); pinMode(PIN_SELECT, INPUT);
  
  // --- CC1101 Data Pins ---
  pinMode(CC_GDO0, INPUT);  // RX Pin
  // pinMode(CC_GDO2, OUTPUT); // TX Pin (Crucial for Playback)
  // digitalWrite(CC_GDO2, LOW);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) for(;;);
  WiFi.mode(WIFI_OFF); display.setTextColor(WHITE);

  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS Mount Failed");
  }

    subghzInit = false;

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
    case AP_MODE:        drawAPMode();       break;
    case SLEEP_MENU:     drawSleepMenu();    break;
    case BLUETOOTH_MENU: drawPlaceholder("BLUETOOTH"); break;
    case IR_MENU:        drawPlaceholder("IR REMOTE"); break;
    
    // --- SUB-GHZ SECTION ---
    case SUBGHZ_MENU:    drawSubGhzMenu();     break;
    case SUBGHZ_SCAN:    handleSubGhzScan();   break;
    case SUBGHZ_RECORD:  handleSubGhzRecord(); break; // <--- NEW
    case SUBGHZ_PLAY:    handleSubGhzPlay();   break; // <--- NEW
    case SUBGHZ_SAVED_LIST: handleSubGhzSavedList(); break;

    default:             drawHomeScreen();     break;
  }

  display.display();
  delay(20);
}