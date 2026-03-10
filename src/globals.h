#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <ESPAsyncWebServer.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>  // <--- This is the correct filename // <--- NEW Library for CC1101
// --- 1. HARDWARE PINS ---
// --- UPDATED HARDWARE PINS (Conflict-Free) ---
#define PIN_UP     27  // Moved from 4 (Optional, but 27 is safer)
#define PIN_DOWN   26  // Moved from 18 (Now used by CC1101 SCK)
#define PIN_SELECT 25  // Moved from 19 (Now used by CC1101 MISO)


// --- CC1101 SPI PINS (Default ESP32 SPI) ---
#define CC_CS    5   // Chip Select
#define CC_GDO0  4   // Input Pin for receiving data
#define CC_GDO2  15  // Optional second data pin

// --- 2. SYSTEM STATES ---
// This enum defines every screen in your "Swiss Army" tool.
enum AppState { 
  HOME, 
  MAIN_MENU, 
  WIFI_MENU, 
  SCANNING, 
  SCAN_WIFI, 
  ENTER_PASS, 
  CONNECTING, 
  BLUETOOTH_MENU, 
  IR_MENU, 
  WIFI_STATUS, 
  SLEEP_MENU, 
  AP_MODE,
  SUBGHZ_MENU,    // <--- NEW
  SUBGHZ_SCAN,    // <--- NEW
  SUBGHZ_RECORD,  // <--- ADD THIS
  SUBGHZ_PLAY     // <--- ADD THIS
};

// --- 3. SHARED SYSTEM OBJECTS ---
// 'extern' tells other files these objects exist in the main file.
extern Adafruit_SSD1306 display;
extern AsyncWebServer server;

// --- 4. SHARED VARIABLES ---
extern AppState currentState;
extern AppState returnState;

// Volatile is required for variables shared with the Web Server core.
extern volatile bool v_up;
extern volatile bool v_down;
extern volatile bool v_sel;
extern volatile bool v_submit;

extern bool wifiPower;
extern bool btPower;
extern bool serverStarted;

extern int menuIdx;
extern int scannedCount;
extern int wifiListIdx;

extern String selectedSSID;
extern String password;
extern String keyboardLabel;
extern String* targetString;

// Keyboard character sets
extern const char* sets[];
extern int currentSet;
extern int charIdx;

extern bool subghzInit; // Track if CC1101 is physically present

#endif