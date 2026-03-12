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
volatile uint32_t pulseBuffer[MAX_PULSES];
volatile int pulseCount = 0;
volatile unsigned long lastTime = 0;

void handleSubGhzSavedList(); // Forward declaration

// --- RECORDING INTERRUPT ---
volatile unsigned long accumulatedNoise = 0;

void IRAM_ATTR subghzRecordISR() {
  if (pulseCount < MAX_PULSES) {
    unsigned long now = micros();
    unsigned long duration = now - lastTime;

    if (lastTime == 0) {
      lastTime = now;
      return;
    }

    // Filter tiny noise spikes (< 50us)
    if (duration < 50) {
      accumulatedNoise += duration;
    } else {
      // If there was noise, we need to effectively ignore it by NOT registering this
      // edge. If we register it, the state flips. Instead, we can add this duration
      // to the *previous* pulse to "fill" the glitch, and decrement pulseCount so the
      // next real edge overwrites this flip.
      if (accumulatedNoise > 0 && pulseCount > 0) {
         pulseBuffer[pulseCount - 1] += duration + accumulatedNoise;
         accumulatedNoise = 0;
      } else {
         pulseBuffer[pulseCount++] = duration + accumulatedNoise;
         accumulatedNoise = 0;
      }
    }
    lastTime = now;
  }
}


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
      ELECHOUSE_cc1101.setPA(12); // MAX Transmit Power
      ELECHOUSE_cc1101.setRxBW(58); // Optimal Receive Bandwidth
      
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
  bool trigger = false;

  // Wait for a strong signal (RSSI > -70)
  while (!trigger) {
    if (digitalRead(PIN_SELECT) == HIGH || v_sel) { 
        v_sel = false; currentState = SUBGHZ_MENU; return; // Cancel
    }

    // RSSI Check: Only trigger if signal is strong (avoids static)
    if (ELECHOUSE_cc1101.getRssi() > -70) {
       trigger = true; 
    }
  }

  // --- PHASE 4: RECORDING LOOP (INTERRUPT DRIVEN) ---
  display.setCursor(0, 50); display.print("CAPTURING...");
  display.display();
  
  lastTime = 0; // Initialize interrupt timer
  pulseCount = 0;
  accumulatedNoise = 0;

  attachInterrupt(digitalPinToInterrupt(CC_GDO0), subghzRecordISR, CHANGE);

  unsigned long startTime = millis();
  while (pulseCount < MAX_PULSES) {
    // Stop if silence for > 50ms (End of packet)
    if (lastTime > 0 && (micros() - lastTime > 50000) && pulseCount > 10) break;

    // Timeout safety
    if (millis() - startTime > 3000) break; // 3 seconds max record
    
    // Safety exit
    if (digitalRead(PIN_SELECT) == HIGH) {
        delay(200); // debounce
        break;
    }
    yield();
  }

  detachInterrupt(digitalPinToInterrupt(CC_GDO0));
  ELECHOUSE_cc1101.setSidle(); // Stop listening
  
  // --- PHASE 5: SAVE/DISCARD ---
  if (pulseCount < 10) {
    display.clearDisplay();
    display.setCursor(0, 10); display.println("CAPTURE FAILED");
    display.setCursor(0, 30); display.println("Signal too weak");
    display.display();
    delay(1500);
    currentState = SUBGHZ_MENU;
    return;
  }

  display.clearDisplay();
  display.setCursor(0, 10); display.println("CAPTURE DONE");
  display.setCursor(0, 30); display.print("Pulses: "); display.println(pulseCount);
  display.setCursor(0, 50); display.println("[UP] Save [DWN] Del");
  display.display();

  while(true) {
    if (digitalRead(PIN_UP) == HIGH || v_up) {
      v_up = false;

      // Auto-generate name based on timestamp or count
      String filename = "/subghz/sig_" + String(millis()) + ".txt";

      // Save logic
      if (!LittleFS.exists("/subghz")) {
         LittleFS.mkdir("/subghz");
      }
      File file = LittleFS.open(filename, FILE_WRITE);
      if (file) {
        file.println(freqList[currentFreqIdx]);
        file.println(modList[currentModIdx]);
        file.println(pulseCount);
        for (int i = 0; i < pulseCount; i++) {
           file.println(pulseBuffer[i]);
        }
        file.close();

        display.clearDisplay();
        display.setCursor(0, 20); display.println("SAVED!");
        display.setCursor(0, 40); display.println(filename);
        display.display();
      } else {
        display.clearDisplay();
        display.setCursor(0, 20); display.println("SAVE FAILED!");
        display.display();
      }
      delay(1500);
      break;
    }
    if (digitalRead(PIN_DOWN) == HIGH || v_down || digitalRead(PIN_SELECT) == HIGH || v_sel) {
      v_down = false; v_sel = false;
      display.clearDisplay();
      display.setCursor(0, 20); display.println("DISCARDED");
      display.display();
      delay(1000);
      break;
    }
  }

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

  // 1. DYNAMIC PIN SWITCH: Use GDO0 as Output
  pinMode(CC_GDO0, OUTPUT);
  digitalWrite(CC_GDO0, LOW);

  // 2. Setup for Transmit
  // Need to make sure frequency and modulation are applied
  ELECHOUSE_cc1101.setMHZ(freqList[currentFreqIdx]);
  ELECHOUSE_cc1101.setModulation(modList[currentModIdx]);
  ELECHOUSE_cc1101.SetTx(); 
  delay(1); // Small delay to let TX state stabilize
  
  // 3. Playback Loop
  // We assume the first pulse recorded was HIGH (because of our trigger)

  // Send the signal a few times (often required by receivers)
  for (int repeat = 0; repeat < 5; repeat++) {
    bool signalState = HIGH;
    for (int i = 0; i < pulseCount; i++) {
      digitalWrite(CC_GDO0, signalState);
      delayMicroseconds(pulseBuffer[i]);
      signalState = !signalState;
    }
    digitalWrite(CC_GDO0, LOW);
    delay(10); // Inter-packet gap
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
    "3. Saved Signals",
    "4. Play Last",
    "5. Back"
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
    else if (menuIdx == 2) {
      menuIdx = 0; // Reset list index
      currentState = SUBGHZ_SAVED_LIST;
    }
    else if (menuIdx == 3) currentState = SUBGHZ_PLAY;
    else if (menuIdx == 4) {
      // Power Down
      ELECHOUSE_cc1101.goSleep(); 
      subghzInit = false; // Reset global flag
      currentState = MAIN_MENU;
    }

    while(digitalRead(PIN_SELECT) == HIGH);
    if(currentState != SUBGHZ_SAVED_LIST) menuIdx = 0;
    delay(300);
  }
}

// --- SAVED SIGNALS MENU ---
void handleSubGhzSavedList() {
  File root = LittleFS.open("/subghz");
  if (!root || !root.isDirectory()) {
    display.clearDisplay();
    display.setCursor(0, 20); display.println("No Saved Signals!");
    display.display();
    delay(1000);
    currentState = SUBGHZ_MENU;
    return;
  }

  // Count files to define array size
  int fileCount = 0;
  File file = root.openNextFile();
  while (file) {
    fileCount++;
    file = root.openNextFile();
  }

  if (fileCount == 0) {
    display.clearDisplay();
    display.setCursor(0, 20); display.println("No Saved Signals!");
    display.display();
    delay(1000);
    currentState = SUBGHZ_MENU;
    return;
  }

  // Reload root to get names safely (no VLAs)
  root = LittleFS.open("/subghz");
  String* filenames = new String[fileCount];
  int i = 0;
  file = root.openNextFile();
  while (file && i < fileCount) {
    filenames[i] = String(file.name());
    file = root.openNextFile();
    i++;
  }

  display.clearDisplay();
  display.setCursor(0, 0); display.println("--- SAVED SIGNALS ---");

  // Back option is at index fileCount
  int totalOpts = fileCount + 1;

  int startIdx = (menuIdx >= 4) ? menuIdx - 3 : 0;
  for (int j = 0; j < 4; j++) {
    int cur = startIdx + j;
    if (cur >= totalOpts) break;
    int y = 15 + (j * 10);

    if (cur == menuIdx) {
      display.fillRect(0, y - 1, 128, 10, WHITE);
      display.setTextColor(BLACK);
    } else {
      display.setTextColor(WHITE);
    }

    if (cur < fileCount) {
      display.setCursor(5, y); display.println(filenames[cur]);
    } else {
      display.setCursor(5, y); display.println("< Back");
    }
  }
  display.setTextColor(WHITE);
  display.display();

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

    if (menuIdx == fileCount) {
      // Go back
      menuIdx = 0;
      currentState = SUBGHZ_MENU;
    } else {
      // Load file and play
      display.clearDisplay();
      display.setCursor(0, 20); display.println("Loading...");
      display.display();

      String path = "/subghz/" + filenames[menuIdx];
      File f = LittleFS.open(path, FILE_READ);
      if (f) {
        String fLine = f.readStringUntil('\n'); fLine.trim();
        String mLine = f.readStringUntil('\n'); mLine.trim();
        String cLine = f.readStringUntil('\n'); cLine.trim();

        float loadedFreq = fLine.toFloat();
        int loadedMod = mLine.toInt();
        pulseCount = cLine.toInt();

        for (int p = 0; p < pulseCount; p++) {
          String pLine = f.readStringUntil('\n'); pLine.trim();
          pulseBuffer[p] = pLine.toInt();
        }
        f.close();

        // Find best freqIdx and modIdx to update UI
        for (int k=0; k<4; k++) if(abs(freqList[k] - loadedFreq) < 0.1) currentFreqIdx = k;
        for (int k=0; k<4; k++) if(modList[k] == loadedMod) currentModIdx = k;

        // Go straight to play
        menuIdx = 0;
        currentState = SUBGHZ_PLAY;
      } else {
        display.clearDisplay();
        display.setCursor(0, 20); display.println("Load Failed!");
        display.display();
        delay(1000);
      }
    }
    while(digitalRead(PIN_SELECT) == HIGH); delay(300);
  }

  // Cleanup dynamically allocated array
  delete[] filenames;
}

#endif