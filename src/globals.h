#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <ESPAsyncWebServer.h>

// --- 1. HARDWARE PINS ---
// Defining these here makes it easy to switch to your S3 board later.
#define PIN_UP 4     
#define PIN_DOWN 18  
#define PIN_SELECT 19 

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
  AP_MODE 
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

#endif