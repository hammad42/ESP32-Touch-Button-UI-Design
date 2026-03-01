#ifndef PLACEHOLDERS_H
#define PLACEHOLDERS_H

#include "globals.h"

/**
 * @brief Draws a temporary screen for unbuilt features.
 * @param title The name of the feature (e.g., "BLUETOOTH").
 */
void drawPlaceholder(const char* title) {
  display.setTextSize(1);
  display.setCursor(30, 30); 
  display.print(title);
  
  display.setCursor(20, 45); 
  display.print("Locked / WIP"); 
  
  display.setCursor(15, 56);
  display.print("[SELECT: BACK]");

  // Input Handling: Exit the placeholder and return to the Main Menu
  if (digitalRead(PIN_SELECT) == HIGH || v_sel) { 
    v_sel = false;
    currentState = MAIN_MENU; 
    while(digitalRead(PIN_SELECT) == HIGH); 
    delay(300); 
  }
}

#endif