#ifndef SLEEP_LOGIC_H
#define SLEEP_LOGIC_H

#include "globals.h"
#include <WiFi.h>

void drawSleepMenu() {
  display.setCursor(0, 10); display.println("   --- SLEEP ---");
  const char* options[] = {"1. Light Sleep", "2. Mod. Sleep", "3. Deep Sleep", "4. Back"};
  
  for (int i = 0; i < 4; i++) {
    int y = 22 + (i * 10);
    if (i == menuIdx) { 
      display.fillRect(0, y-1, 128, 10, WHITE); 
      display.setTextColor(BLACK); 
    } else {
      display.setTextColor(WHITE);
    }
    display.setCursor(5, y); display.println(options[i]);
  }
  display.setTextColor(WHITE);

  // --- INPUT HANDLING ---
  if (digitalRead(PIN_DOWN) == HIGH || v_down) { v_down = false; menuIdx = (menuIdx + 1) % 4; while(digitalRead(PIN_DOWN) == HIGH); delay(250); }
  if (digitalRead(PIN_UP) == HIGH || v_up)   { v_up = false; menuIdx = (menuIdx - 1 + 4) % 4; while(digitalRead(PIN_UP) == HIGH); delay(250); }
  
  if (digitalRead(PIN_SELECT) == HIGH || v_sel) {
    v_sel = false;
    
    if (menuIdx == 0) {
      // 1. LIGHT SLEEP: RAM stays alive, but CPU pauses. Wakes up on button press.
      display.clearDisplay(); display.setCursor(30, 30); display.print("Light Sleep"); display.display();
      delay(1000);
      gpio_wakeup_enable((gpio_num_t)PIN_SELECT, GPIO_INTR_HIGH_LEVEL);
      esp_sleep_enable_gpio_wakeup();
      esp_light_sleep_start();
      Wire.begin(); // Re-init I2C for the OLED
      currentState = HOME;
    }
    else if (menuIdx == 1) {
      // 2. MODEM SLEEP: Shuts down WiFi/Bluetooth to save 80% power.
      display.clearDisplay(); display.setCursor(20, 30); display.print("Modem Sleep"); display.display();
      delay(1000);
      WiFi.disconnect();
      WiFi.mode(WIFI_OFF);
      wifiPower = false;
      btPower = false;
      display.ssd1306_command(SSD1306_DISPLAYOFF); // Turn off OLED pixels
      while(digitalRead(PIN_SELECT) == LOW) { delay(100); } // Wait for manual wake
      display.ssd1306_command(SSD1306_DISPLAYON);
      currentState = HOME;
    }
    else if (menuIdx == 2) {
      // 3. DEEP SLEEP: Only RTC stays alive. Device "reboots" to wake up.
      display.clearDisplay(); display.setCursor(30, 30); display.print("Deep Sleep"); 
      display.setCursor(15, 45); display.print("Press UP to Wake"); display.display();
      delay(2000);
      esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_UP, 1); 
      esp_deep_sleep_start();
    }
    else if (menuIdx == 3) {
      currentState = MAIN_MENU;
    }
    
    while(digitalRead(PIN_SELECT) == HIGH);
    menuIdx = 0; delay(300);
  }
}

#endif