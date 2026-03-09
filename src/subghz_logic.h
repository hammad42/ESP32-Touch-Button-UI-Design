#ifndef SUBGHZ_LOGIC_H
#define SUBGHZ_LOGIC_H

#include "globals.h"

// --- 1. CONFIGURATION ARRAYS ---
// The options available to toggle through
float freqList[] = {433.92, 315.00, 868.35, 915.00};
const char* freqNames[] = {"433.92", "315.00", "868.35", "915.00"};

int modList[] = {2, 0}; // 2=ASK/OOK (Remotes), 0=2-FSK (Data/Cars)
const char* modNames[] = {"ASK", "FSK"};

// --- GLOBAL VARIABLES ---
// Store the current selection here so it remembers it
int currentFreqIdx = 0; // Default to 433.92
int currentModIdx = 0;  // Default to ASK
bool subghzStarted = false;


// --- INITIALIZATION FUNCTION ---
void initSubGhz() {
  if (!subghzStarted) {
    display.clearDisplay();
    display.setCursor(0, 20); display.println("Starting Radio...");
    display.display();

    // 1. Define Pins
    ELECHOUSE_cc1101.setSpiPin(18, 19, 23, 5); 
    ELECHOUSE_cc1101.setGDO0(CC_GDO0);         

    // 2. Attempt Detection
    if (ELECHOUSE_cc1101.getCC1101()) {
      ELECHOUSE_cc1101.Init();
      // Use the variables instead of hardcoded numbers
      ELECHOUSE_cc1101.setMHZ(freqList[currentFreqIdx]);    
      ELECHOUSE_cc1101.setModulation(modList[currentModIdx]);  
      ELECHOUSE_cc1101.setDRate(512); 
      
      subghzStarted = true;
      Serial.println("Sub-GHz Radio Initialized.");
    } else {
      display.setCursor(0, 35); display.println("Radio Not Found!");
      display.display();
      delay(2000);
    }
  }
}

// --- FREQUENCY ANALYZER (The "Scanner") ---
void handleSubGhzScan() {
  // 1. Header
  display.setCursor(0, 0); display.println("--- ANALYZER ---");
  
  // 2. Display Config (Visible on Screen)
  display.setCursor(0, 12);
  display.print("Frq: "); display.print(freqNames[currentFreqIdx]); display.print(" [UP]");

  display.setCursor(0, 22);
  display.print("Mod: "); display.print(modNames[currentModIdx]); display.print("    [DWN]");

  // 3. Radio Logic
  ELECHOUSE_cc1101.SetRx(); 
  int rssi = ELECHOUSE_cc1101.getRssi();
  
  // 4. Display RSSI
  display.setCursor(0, 35);
  display.print("RSSI: "); display.print(rssi); display.println(" dBm");

  // 5. Visual Bar
  if (rssi >= -100) { 
    int barWidth = map(constrain(rssi, -100, -20), -100, -20, 0, 100);
    display.drawRect(10, 48, 108, 6, WHITE);
    display.fillRect(12, 50, barWidth, 2, WHITE);
    
    // Peak Detector
    if (rssi > -60) {
       display.setCursor(10, 56); display.println("SIGNAL FOUND!");
    }
  }

  // --- CONTROLS ---

  // UP Button: Change Frequency
  if (digitalRead(PIN_UP) == HIGH || v_up) {
    v_up = false;
    currentFreqIdx = (currentFreqIdx + 1) % 4; // Cycle 0-3
    
    // Apply New Settings Immediately
    ELECHOUSE_cc1101.setMHZ(freqList[currentFreqIdx]);
    ELECHOUSE_cc1101.setModulation(modList[currentModIdx]); 
    ELECHOUSE_cc1101.SetRx(); // Restart RX with new settings
    
    delay(200); // Debounce
  }
  
  // DOWN Button: Change Modulation
  if (digitalRead(PIN_DOWN) == HIGH || v_down) {
    v_down = false;
    currentModIdx = (currentModIdx + 1) % 2; // Cycle 0-1
    
    // Apply New Settings Immediately
    ELECHOUSE_cc1101.setMHZ(freqList[currentFreqIdx]);
    ELECHOUSE_cc1101.setModulation(modList[currentModIdx]);
    ELECHOUSE_cc1101.SetRx(); 
    
    delay(200); 
  }

  // SELECT Button: Back
  if (digitalRead(PIN_SELECT) == HIGH || v_sel) { 
    v_sel = false; 
    ELECHOUSE_cc1101.setSidle(); 
    currentState = SUBGHZ_MENU; 
    delay(300); 
  }
}

// --- MAIN SUB-GHZ MENU ---
void drawSubGhzMenu() {
  initSubGhz(); // Ensure radio is on

  display.setCursor(0, 15); display.println("   --- SUB-GHZ ---");
  
  const char* options[] = {"1. Freq Analyzer", "2. Back"};
  int totalOpts = 2;

  int startIdx = (menuIdx >= 4) ? menuIdx - 3 : 0;
  for (int i = 0; i < 4; i++) {
    int cur = startIdx + i; 
    if (cur >= totalOpts) break;
    int y = 25 + (i * 10);
    
    if (cur == menuIdx) { 
      display.fillRect(0, y - 1, 128, 10, WHITE); 
      display.setTextColor(BLACK); 
    } else {
      display.setTextColor(WHITE);
    }
    display.setCursor(5, y); display.println(options[cur]);
  }
  display.setTextColor(WHITE);

  if (digitalRead(PIN_DOWN) == HIGH || v_down) { 
    v_down = false; menuIdx = (menuIdx + 1) % totalOpts; 
    while(digitalRead(PIN_DOWN) == HIGH); delay(200); 
  }
  
  if (digitalRead(PIN_UP) == HIGH || v_up) { 
    v_up = false; menuIdx = (menuIdx - 1 + totalOpts) % totalOpts; 
    while(digitalRead(PIN_UP) == HIGH); delay(200); 
  }

  if (digitalRead(PIN_SELECT) == HIGH || v_sel) {
    v_sel = false;
    if (menuIdx == 0)      currentState = SUBGHZ_SCAN;  
    else if (menuIdx == 1) currentState = MAIN_MENU;

    while(digitalRead(PIN_SELECT) == HIGH);
    menuIdx = 0; delay(300);
  }
}

#endif