#ifndef CONNECTION_LOGIC_H
#define CONNECTION_LOGIC_H

#include "globals.h"
#include <WiFi.h>

void handleConnection() {
  display.clearDisplay(); display.setCursor(20, 30); display.print("Connecting...");
  display.setCursor(20, 42); display.print(selectedSSID.substring(0, 15)); display.display();

  if (WiFi.getMode() & WIFI_AP) WiFi.mode(WIFI_AP_STA); else WiFi.mode(WIFI_STA);

  WiFi.begin(selectedSSID.c_str(), password.c_str());
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500); attempts++;
    display.fillRect(20, 52, attempts * 4, 3, WHITE); display.display();
  }
  
  display.clearDisplay(); display.setCursor(20, 30);
  if (WiFi.status() == WL_CONNECTED) {
    display.println("CONNECTED!");
    display.setCursor(20, 42); display.println(WiFi.localIP().toString());
  } else {
    display.println("FAILED");
    WiFi.disconnect();
  }
  display.display(); delay(2000); currentState = HOME;
}

void drawWiFiStatus() {
  display.setCursor(0, 15); display.println("--- CONN. STATUS ---");
  if(WiFi.status() == WL_CONNECTED) {
    display.setCursor(0, 28);
    display.print("SSID: "); display.println(WiFi.SSID());
    display.print("IP  : "); display.println(WiFi.localIP().toString());
  } else if (WiFi.getMode() & WIFI_AP) {
    display.setCursor(0, 28); display.print("AP SSID: "); display.println("SWISS_ARMY");
    display.print("Clients: "); display.println(WiFi.softAPgetStationNum());
  } else {
    display.setCursor(0, 35); display.println("Status: IDLE");
  }
  display.setCursor(20, 56); display.print("[SELECT: BACK]");
  if (digitalRead(PIN_SELECT) == HIGH || v_sel) { v_sel = false; currentState = WIFI_MENU; while(digitalRead(PIN_SELECT) == HIGH); delay(300); }
}

#endif