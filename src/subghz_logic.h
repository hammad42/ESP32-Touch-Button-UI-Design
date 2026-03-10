#ifndef SUBGHZ_LOGIC_H
#define SUBGHZ_LOGIC_H

#include "globals.h"

// --- 1. CONFIGURATION ARRAYS ---
float freqList[] = {433.92, 315.00, 868.35, 915.00};
const char* freqNames[] = {"433.92", "315.00", "868.35", "915.00"};

int modList[] = {2, 0, 1, 3}; // ASK, 2FSK, GFSK, 4FSK
const char* modNames[] = {"ASK", "2FSK", "GFSK", "4FSK"};

// --- GLOBAL VARIABLES FOR LOGIC ---
int currentFreqIdx = 0; 
int currentModIdx = 0;  

// --- RECORDING BUFFER ---
#define MAX_PULSES 512
uint32_t pulseBuffer[MAX_PULSES];
int pulseCount = 0;


// --- INITIALIZATION FUNCTION ---
void initSubGhz() {
  // Use 'subghzInit' from globals.h
  if (!subghzInit) {
    display.clearDisplay();
    display.setCursor(0, 20); display.println("Starting Radio...");
    display.display();

    ELECHOUSE_cc1101.setSpiPin(18, 19, 23, 5); 
    ELECHOUSE_cc1101.setGDO0(CC_GDO0);         

    if (ELECHOUSE_cc1101.getCC1101()) {
      ELECHOUSE_cc1101.Init();
      ELECHOUSE_cc1101.setMHZ(freqList[currentFreqIdx]);    
      ELECHOUSE_cc1101.setModulation(modList[currentModIdx]);  
      ELECHOUSE_cc1101.setDRate(512); 
      
      subghzInit = true; // Set the global flag
      Serial.println("Sub-GHz Radio Initialized.");
    } else {
      display.setCursor(0, 35); display.println("Radio Not Found!");
      display.display();
      delay(2000);
    }
  }
}

// --- 1. FREQUENCY ANALYZER ---
void handleSubGhzScan() {
  display.setCursor(0, 0); display.println("--- ANALYZER ---");
  
  display.setCursor(0, 12);
  display.print("Frq: "); display.print(freqNames[currentFreqIdx]); display.print(" [UP]");
  display.setCursor(0, 22);
  display.print("Mod: "); display.print(modNames[currentModIdx]); display.print("    [DWN]");

  ELECHOUSE_cc1101.SetRx(); 
  int rssi = ELECHOUSE_cc1101.getRssi();
  
  display.setCursor(0, 35);
  display.print("RSSI: "); display.print(rssi); display.println(" dBm");

  if (rssi >= -100) { 
    int barWidth = map(constrain(rssi, -100, -20), -100, -20, 0, 100);
    display.drawRect(10, 48, 108, 6, WHITE);
    display.fillRect(12, 50, barWidth, 2, WHITE);
    if (rssi > -60) display.setCursor(10, 56); display.println("SIGNAL FOUND!");
  }

  // Controls
  if (digitalRead(PIN_UP) == HIGH || v_up) {
    v_up = false;
    currentFreqIdx = (currentFreqIdx + 1) % 4; 
    ELECHOUSE_cc1101.setMHZ(freqList[currentFreqIdx]);
    ELECHOUSE_cc1101.setModulation(modList[currentModIdx]); 
    ELECHOUSE_cc1101.SetRx(); 
    delay(200); 
  }
  
  if (digitalRead(PIN_DOWN) == HIGH || v_down) {
    v_down = false;
    currentModIdx = (currentModIdx + 1) % 4; 
    ELECHOUSE_cc1101.setMHZ(freqList[currentFreqIdx]);
    ELECHOUSE_cc1101.setModulation(modList[currentModIdx]);
    ELECHOUSE_cc1101.SetRx(); 
    delay(200); 
  }

  if (digitalRead(PIN_SELECT) == HIGH || v_sel) { 
    v_sel = false; 
    ELECHOUSE_cc1101.setSidle(); 
    currentState = SUBGHZ_MENU; 
    delay(300); 
  }
}

// --- 2. RECORD SIGNAL (With Config Screen & Smart Trigger) ---
void handleSubGhzRecord() {
  // --- PHASE 1: CONFIGURATION SCREEN ---
  bool configDone = false;
  
  // Use the global indexes so we remember what you set last time
  while (!configDone) {
    display.clearDisplay();
    display.setCursor(0, 0); display.println("--- REC SETUP ---");
    
    // Show current settings
    display.setCursor(0, 20);
    display.print("Freq: "); display.println(freqNames[currentFreqIdx]);
    
    display.setCursor(0, 30);
    display.print("Mod:  "); display.println(modNames[currentModIdx]);
    
    display.setCursor(0, 50); display.println("[SEL] TO START >");
    display.display();

    // INPUT: UP changes Freq
    if (digitalRead(PIN_UP) == HIGH || v_up) {
      v_up = false;
      currentFreqIdx = (currentFreqIdx + 1) % 4; 
      delay(200); 
    }

    // INPUT: DOWN changes Modulation
    if (digitalRead(PIN_DOWN) == HIGH || v_down) {
      v_down = false;
      currentModIdx = (currentModIdx + 1) % 4; 
      delay(200); 
    }

    // INPUT: SELECT confirms settings and starts
    if (digitalRead(PIN_SELECT) == HIGH || v_sel) {
      v_sel = false;
      configDone = true; // Exit loop and start recording
      delay(300);
    }
  }

  // --- PHASE 2: APPLY SETTINGS ---
  display.clearDisplay();
  display.setCursor(0, 20); display.println("Configuring Radio...");
  display.display();

  // Apply the values you just selected!
  ELECHOUSE_cc1101.setMHZ(freqList[currentFreqIdx]); 
  ELECHOUSE_cc1101.setModulation(modList[currentModIdx]); 
  ELECHOUSE_cc1101.SetRx(); 

  // --- PHASE 3: SMART TRIGGER (WAIT FOR SIGNAL) ---
  display.clearDisplay();
  display.setCursor(0, 0); display.println("--- ARMED ---");
  display.setCursor(0, 20); display.print("F:"); display.println(freqNames[currentFreqIdx]);
  display.setCursor(0, 30); display.println("Waiting for Sync...");
  display.display();

  pulseCount = 0;
  bool lastState = LOW;
  unsigned long lastChangeTime = micros();
  bool trigger = false;

  // Wait for a strong signal (RSSI > -60)
  while (!trigger) {
    if (digitalRead(PIN_SELECT) == HIGH || v_sel) { 
        v_sel = false; currentState = SUBGHZ_MENU; return; // Cancel
    }

    // RSSI Check: Only trigger if signal is strong (avoids static)
    if (ELECHOUSE_cc1101.getRssi() > -70) {
       trigger = true; 
    }
  }

  // --- PHASE 4: RECORDING LOOP ---
  display.setCursor(0, 50); display.print("CAPTURING...");
  display.display();
  
  lastChangeTime = micros(); // Reset timer at start of capture
  lastState = digitalRead(CC_GDO0);

  while (pulseCount < MAX_PULSES) {
    bool currentState = digitalRead(CC_GDO0);
    
    if (currentState != lastState) {
      unsigned long now = micros();
      unsigned long duration = now - lastChangeTime;
      
      // Filter tiny noise spikes (< 50us)
      if (duration > 50) { 
        pulseBuffer[pulseCount++] = duration;
        lastChangeTime = now;
        lastState = currentState;
      } else {
        lastChangeTime = now; 
      }
    }

    // Stop if silence for > 50ms (End of packet)
    if (micros() - lastChangeTime > 50000 && pulseCount > 10) break;
    
    // Safety exit
    if (digitalRead(PIN_SELECT) == HIGH) break;
  }

  ELECHOUSE_cc1101.setSidle(); // Stop listening
  
  // --- PHASE 5: SAVE/DISCARD ---
  display.clearDisplay();
  display.setCursor(0, 10); display.println("CAPTURE DONE");
  display.setCursor(0, 30); display.print("Pulses: "); display.println(pulseCount);
  display.setCursor(0, 50); display.println("[SEL] OK");
  display.display();

  while(digitalRead(PIN_SELECT) == LOW); 
  delay(300);
  currentState = SUBGHZ_MENU;
}

// --- 3. PLAY SIGNAL (Cloner) ---
void handleSubGhzPlay() {
  if (pulseCount == 0) {
    display.clearDisplay();
    display.setCursor(0, 20); display.println("No Signal Stored!");
    display.display();
    delay(1000);
    currentState = SUBGHZ_MENU;
    return;
  }

  display.clearDisplay();
  display.setCursor(0, 20); display.println("TRANSMITTING...");
  display.setCursor(0, 40); display.println("Sending Signal...");
  display.display();

  // 1. Setup for Transmit
  ELECHOUSE_cc1101.SetTx(); 
  
  // 2. DYNAMIC PIN SWITCH: Use GDO0 as Output
  pinMode(CC_GDO0, OUTPUT); 

  // 3. Playback Loop
  // We assume the first pulse recorded was HIGH (because of our trigger)
  bool signalState = HIGH; 
  for (int i = 0; i < pulseCount; i++) {
    digitalWrite(CC_GDO0, signalState); 
    delayMicroseconds(pulseBuffer[i]);
    signalState = !signalState; 
  }
  
  // 4. Reset Pin
  digitalWrite(CC_GDO0, LOW);
  pinMode(CC_GDO0, INPUT); // Set back to Input for RX
  ELECHOUSE_cc1101.setSidle(); 
  
  display.clearDisplay();
  display.setCursor(0, 30); display.println("Done!");
  display.display();
  delay(1000);
  currentState = SUBGHZ_MENU;
}

// --- MAIN SUB-GHZ MENU ---
void drawSubGhzMenu() {
  initSubGhz(); 

  display.setCursor(0, 15); display.println("   --- SUB-GHZ ---");
  
  const char* options[] = {
    "1. Freq Analyzer", 
    "2. Record Signal",
    "3. Play Signal",
    "4. Back"
  };
  int totalOpts = 4;

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
    else if (menuIdx == 1) currentState = SUBGHZ_RECORD;
    else if (menuIdx == 2) currentState = SUBGHZ_PLAY;
    else if (menuIdx == 3) { 
      // Power Down
      ELECHOUSE_cc1101.goSleep(); 
      subghzInit = false; // Reset global flag
      currentState = MAIN_MENU;
    }

    while(digitalRead(PIN_SELECT) == HIGH);
    menuIdx = 0; delay(300);
  }
}

#endif