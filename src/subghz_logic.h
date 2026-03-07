#ifndef SUBGHZ_LOGIC_H
#define SUBGHZ_LOGIC_H

#include "globals.h"

// --- 1. FREQUENCY ANALYZER (The "Sniffer") ---
void handleSubGhzScan() {
  display.setCursor(0, 15); display.println("--- SUB-GHZ SCAN ---");
  
  // Update Frequency and Read RSSI
    ELECHOUSE_cc1101.SetRx(); // Start Listening
    if (signal) {
      int rssi = ELECHOUSE_cc1101.getRssi();
      display.setCursor(0, 42);
        display.print("RSSI: "); display.print(rssi); display.println(" dBm");
        int barWidth = map(constrain(rssi, -100, -20), -100, -20, 0, 100);
  display.drawRect(10, 52, 104, 6, WHITE);
  display.fillRect(12, 54, barWidth, 2, WHITE);
    } else {
      display.setCursor(0, 42); display.println("No Signal Detected");
    }
  
  
  

  // Visual Signal Strength Bar
  

  if (digitalRead(PIN_SELECT) == HIGH || v_sel) { 
    v_sel = false; currentState = SUBGHZ_MENU; delay(300); 
  }
}

// --- 2. BRUTE FORCE (The "Auditor") ---
void handleSubGhzBrute() {
    display.setCursor(0, 15); display.println("--- BRUTE FORCE ---");
    display.setCursor(0, 30); display.println("Not Implemented Yet");
    display.setCursor(0, 45); display.print("[SELECT: BACK]");
    
    if (digitalRead(PIN_SELECT) == HIGH || v_sel) { 
        v_sel = false; currentState = SUBGHZ_MENU; delay(300); 
    }
}

// --- 3. SUB-GHZ MENU ---
void drawSubGhzMenu() {
  display.setCursor(0, 15); display.println("   --- SUB-GHZ ---");
  
  // Added Raw Sniffer and Hardware Check to the list
  const char* options[] = {
    "1. Freq Analyzer", 
    "2. Raw Sniffer", 
    "3. Brute Force", 
    "4. waveform", 
    "5. Back"
  };
  int totalOpts = 5; // Updated count

  // Scroll logic for 128x64 display (shows 4 items at a time)
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

  // --- INPUT HANDLING ---
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
    
    // Updated state transitions
    if (menuIdx == 0)      currentState = SUBGHZ_SCAN;
    else if (menuIdx == 1) currentState = SUBGHZ_READ;  // Points to handleSubGhzRaw()
    else if (menuIdx == 2) currentState = SUBGHZ_BRUTE; // Points to handleSubGhzBrute()
    else if (menuIdx == 3) currentState = SUBGHZ_WAVE;  // Points to handleSubGhzWaveform()
    else if (menuIdx == 4) currentState = MAIN_MENU;

    while(digitalRead(PIN_SELECT) == HIGH);
    menuIdx = 0; delay(300);
  }
}

void handleSubGhzRaw() {
  display.setCursor(0, 15); display.println("--- RAW CAPTURE ---");
  display.setCursor(0, 25); display.println("Waiting for GDO0...");
  
  // 1. Put the chip into Receive (RX) Mode
  ELECHOUSE_cc1101.SetRx(); 
  
  unsigned long startTime = micros();
  bool lastState = digitalRead(CC_GDO0);
  int pulseCount = 0;

  // 2. Capture loop (Runs for 2 seconds or until 100 pulses)
  while (millis() - startTime / 1000 < 2000 && pulseCount < 100) {
    bool currentState = digitalRead(CC_GDO0);
    
    if (currentState != lastState) {
      unsigned long duration = micros() - startTime;
      startTime = micros();
      
      // Print pulse length to Serial Monitor for analysis
      Serial.print(lastState ? "HIGH: " : "LOW: ");
      Serial.println(duration);
      
      lastState = currentState;
      pulseCount++;
    }
    
    // Allow exit via button
    if (digitalRead(PIN_SELECT) == HIGH) break;
  }

  
  display.setCursor(0, 45); display.print("Pulses: "); display.println(pulseCount);
  display.setCursor(0, 56); display.print("[SELECT: BACK]");

  if (digitalRead(PIN_SELECT) == HIGH || v_sel) { 
    v_sel = false; currentState = SUBGHZ_MENU; delay(300); 
  }
}

void handleSubGhzWaveform() {
  display.clearDisplay();
  display.setCursor(0, 0); 
  display.println("RAW SNIFFER - 433.92");
  
  ELECHOUSE_cc1101.SetRx(); // Start Listening
  
  int xPos = 0;
  while (xPos < 128) {
    bool signal = digitalRead(CC_GDO0);
    if (signal) display.drawLine(xPos, 45, xPos, 25, WHITE); 
    else display.drawPixel(xPos, 45, WHITE);

    xPos++;
    delayMicroseconds(500);
    if (digitalRead(PIN_SELECT) == HIGH) break;
  }

  display.setCursor(0, 55);
  display.print("RSSI: "); display.print(ELECHOUSE_cc1101.getRssi());
  display.display();

  if (digitalRead(PIN_SELECT) == HIGH || v_sel) { 
    v_sel = false; 
    ELECHOUSE_cc1101.setSidle(); // <--- Corrected name here
    currentState = SUBGHZ_MENU; 
    delay(300); 
  }
}

#endif