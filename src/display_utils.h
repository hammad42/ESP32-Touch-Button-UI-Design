#ifndef DISPLAY_UTILS_H
#define DISPLAY_UTILS_H

#include "globals.h"
#include <WiFi.h>

// --- 1. STATUS BAR ---
// Draws the icons for WiFi Station, Access Point, and Bluetooth
void drawStatusBar() {
  // WiFi Station Status (STA)
  if (wifiPower) {
    if (WiFi.status() == WL_CONNECTED) {
      display.fillRect(115, 1, 8, 8, WHITE); // Solid = Connected
    } else {
      display.drawRect(115, 1, 8, 8, WHITE); // Hollow = Power On
    }
  }

  // Access Point Status (AP)
  if (WiFi.getMode() & WIFI_AP) { 
    display.setCursor(102, 1);
    display.print("A");
    // Heartbeat for AP clients
    if (WiFi.softAPgetStationNum() > 0 && (millis() / 500) % 2 == 0) {
       display.drawPixel(108, 1, WHITE); 
    }
  }

  // Bluetooth Power Status
  if (btPower) { 
    display.setCursor(90, 1); 
    display.print("B"); 
  }

  display.drawFastHLine(0, 11, 128, WHITE);
}

// --- 2. HOME SCREEN ---
// Displays real-time CPU, RAM, and Uptime telemetry
void drawHomeScreen() {
  display.setTextSize(1);
  
  // CPU and Revision Data
  display.setCursor(0, 16);
  display.print("CPU:"); display.print(ESP.getCpuFreqMHz()); display.print("M");
  display.setCursor(65, 16);
  display.print("REV:"); display.print(ESP.getChipRevision());

  display.drawFastHLine(0, 26, 128, WHITE);

  // Memory Telemetry (Crucial for monitoring stability)
  uint32_t freeHeap = ESP.getFreeHeap();
  display.setCursor(0, 30);
  display.print("HEAP:"); display.print(freeHeap / 1024); display.print("K");
  
  int usedPct = 100 - ((freeHeap * 100) / ESP.getHeapSize());
  display.setCursor(65, 30);
  display.print("USE:"); display.print(usedPct); display.print("%");

  display.setCursor(0, 40);
  display.print("MIN :"); display.print(ESP.getMinFreeHeap() / 1024); display.print("K");
  display.setCursor(65, 40);
  display.print("FLS:"); display.print(ESP.getFlashChipSize() / 1024 / 1024); display.print("M");

  display.drawFastHLine(0, 50, 128, WHITE);

  // Uptime Clock and Heartbeat
  long t = millis() / 1000;
  display.setCursor(0, 54);
  display.printf("%02d:%02d:%02d", (int)(t/3600), (int)((t%3600)/60), (int)(t%60));

  if((millis() / 500) % 2 == 0) {
    display.setCursor(110, 54);
    display.write(3); // Heart icon
  }

  // Input Handling: Go to Main Menu
  if (digitalRead(PIN_SELECT) == HIGH || v_sel) { 
    v_sel = false;
    currentState = MAIN_MENU; 
    while(digitalRead(PIN_SELECT) == HIGH); 
    delay(300); 
  }
}

#endif