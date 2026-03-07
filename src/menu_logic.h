#ifndef MENU_LOGIC_H
#define MENU_LOGIC_H

#include "globals.h"

// --- 1. MAIN SYSTEM MENU ---
void drawMainMenu() {
  // Added Sub-GHz to the list
  const char* options[] = {"1. WiFi", "2. Bluetooth", "3. IR", "4. Sub-GHz", "5. Sleep", "6. Exit"};
  int totalMainOpts = 6;

  for (int i = 0; i < totalMainOpts; i++) {
    int y = 15 + (i * 10);
    // Highlight logic
    if (i == menuIdx) { 
      display.fillRect(0, y-1, 128, 10, WHITE); 
      display.setTextColor(BLACK); 
    } else {
      display.setTextColor(WHITE);
    }
    display.setCursor(5, y); 
    display.println(options[i]);
  }
  display.setTextColor(WHITE);

  // --- INPUT HANDLING ---
  if (digitalRead(PIN_DOWN) == HIGH || v_down) { 
    v_down = false; 
    menuIdx = (menuIdx + 1) % totalMainOpts; 
    while(digitalRead(PIN_DOWN) == HIGH); delay(250); 
  }

  if (digitalRead(PIN_UP) == HIGH || v_up) { 
    v_up = false; 
    menuIdx = (menuIdx - 1 + totalMainOpts) % totalMainOpts; 
    while(digitalRead(PIN_UP) == HIGH); delay(250); 
  }

  if (digitalRead(PIN_SELECT) == HIGH || v_sel) {
    v_sel = false;
    // Updated switch logic for new menu order
    if (menuIdx == 0) currentState = WIFI_MENU;
    else if (menuIdx == 1) currentState = BLUETOOTH_MENU;
    else if (menuIdx == 2) currentState = IR_MENU;
    else if (menuIdx == 3) currentState = SUBGHZ_MENU; // <--- NEW: Switch to Sub-GHz
    else if (menuIdx == 4) currentState = SLEEP_MENU;
    else if (menuIdx == 5) currentState = HOME;
    
    while(digitalRead(PIN_SELECT) == HIGH); 
    menuIdx = 0; // Reset for the next screen
    delay(300);
  }
}

// --- 2. WIFI CONFIGURATION MENU ---
void drawWiFiMenu() {
  display.setCursor(0, 15); display.println("   --- WIFI ---");
  const char* options[] = { 
    wifiPower ? "Power: ON" : "Power: OFF", 
    "Start Scanning", 
    "Access Point (Remote)", 
    "Connection Status", 
    "Back" 
  };
  int totalOpts = 5;

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
    display.setCursor(5, y); 
    display.println(options[cur]);
  }
  display.setTextColor(WHITE);

  if (digitalRead(PIN_DOWN) == HIGH || v_down) { 
    v_down = false; 
    menuIdx = (menuIdx + 1) % totalOpts; 
    while(digitalRead(PIN_DOWN) == HIGH); delay(200); 
  }
  
  if (digitalRead(PIN_UP) == HIGH || v_up) { 
    v_up = false; 
    menuIdx = (menuIdx - 1 + totalOpts) % totalOpts; 
    while(digitalRead(PIN_UP) == HIGH); delay(200); 
  }

  if (digitalRead(PIN_SELECT) == HIGH || v_sel) {
    v_sel = false;
    if (menuIdx == 0) { 
      wifiPower = !wifiPower; 
      if(wifiPower) WiFi.mode(WIFI_STA); else WiFi.mode(WIFI_OFF); 
    }
    else if (menuIdx == 1 && wifiPower) currentState = SCANNING;
    else if (menuIdx == 2) currentState = AP_MODE;
    else if (menuIdx == 3) currentState = WIFI_STATUS;
    else if (menuIdx == 4) currentState = MAIN_MENU;

    while(digitalRead(PIN_SELECT) == HIGH);
    menuIdx = 0; delay(300);
  }
}

#endif