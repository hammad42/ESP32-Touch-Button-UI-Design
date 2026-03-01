#ifndef KEYBOARD_LOGIC_H
#define KEYBOARD_LOGIC_H

#include "globals.h"

void drawKeyboard() {
  if (targetString == nullptr) { currentState = MAIN_MENU; return; }
  const char* activeSet = sets[currentSet];
  int len = strlen(activeSet);
  
  display.setTextSize(1);
  display.setCursor(0, 13); display.print(keyboardLabel);
  
  // Real-time Sync: Text from phone appears here instantly
  display.setCursor(0, 23); display.print("VAL: "); display.print(*targetString); display.println("_");
  display.drawFastHLine(0, 31, 128, WHITE);

  // Drawing the scrolling character carousel
  display.setCursor(15, 45); display.print(activeSet[(charIdx - 2 + len) % len]);
  display.setCursor(40, 45); display.print(activeSet[(charIdx - 1 + len) % len]);
  display.setTextSize(2); display.setCursor(58, 41); display.print(activeSet[charIdx]);
  display.drawRoundRect(54, 38, 22, 22, 3, WHITE);
  display.setTextSize(1);
  display.setCursor(88, 45); display.print(activeSet[(charIdx + 1) % len]);
  display.setCursor(113, 45); display.print(activeSet[(charIdx + 2) % len]);

  // --- DUAL CONTROL: DOWN (Next Character) ---
  if (digitalRead(PIN_DOWN) == HIGH || v_down) { 
    v_down = false; 
    charIdx = (charIdx + 1) % len; 
    while(digitalRead(PIN_DOWN) == HIGH); 
    delay(200); 
  }

  // --- DUAL CONTROL: UP (Prev Character / Set Change) ---
  if (digitalRead(PIN_UP) == HIGH || v_up) {
    v_up = false;
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

  // --- DUAL CONTROL: SELECT / SUBMIT ---
  if (digitalRead(PIN_SELECT) == HIGH || v_sel || v_submit) {
    unsigned long start = millis();
    bool isRemoteSubmit = v_submit;
    v_submit = false; 
    
    if (digitalRead(PIN_SELECT) == HIGH) while(digitalRead(PIN_SELECT) == HIGH); 
    
    unsigned long dur = (v_sel || isRemoteSubmit) ? 0 : (millis() - start);
    v_sel = false;

    if (dur > 2500 || isRemoteSubmit) { 
      currentState = returnState; 
      delay(500); 
    }
    else if (dur > 800) { 
      if ((*targetString).length() > 0) (*targetString).remove((*targetString).length() - 1); 
      delay(300); 
    }
    else { 
      char selectedChar = activeSet[charIdx];
      if (selectedChar == '<') { 
        if ((*targetString).length() > 0) (*targetString).remove((*targetString).length() - 1); 
      } 
      else if (selectedChar == 'X') currentState = WIFI_MENU; 
      else *targetString += selectedChar;
      delay(300); 
    }
  }
}

#endif