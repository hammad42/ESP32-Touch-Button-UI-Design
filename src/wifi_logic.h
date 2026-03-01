#ifndef WIFI_LOGIC_H
#define WIFI_LOGIC_H

#include "globals.h"
#include "webpages.h"
#include <WiFi.h>

// --- 1. ACCESS POINT & REMOTE CONTROL ---
void drawAPMode() {
  if (!serverStarted) {
    WiFi.softAP("SWISS_ARMY_ESP", NULL);
    
    // Serve the HTML from webpages.h
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *r){ 
      r->send_P(200, "text/html", index_html); 
    });

    // Handle Remote Buttons
    server.on("/ctrl", HTTP_GET, [](AsyncWebServerRequest *r){ 
      String c = r->getParam("c")->value();
      if(c=="up") v_up=true; if(c=="down") v_down=true; if(c=="sel") v_sel=true;
      r->send(200); 
    });

    // Handle Remote Text Injection
    server.on("/text", HTTP_GET, [](AsyncWebServerRequest *r){
      if(r->hasParam("v") && targetString) *targetString = r->getParam("v")->value();
      r->send(200);
    });

    // Handle Remote Submit
    server.on("/submit", HTTP_GET, [](AsyncWebServerRequest *r){ 
      v_submit = true; r->send(200); 
    });

    // The Screen Mirroring Engine
    server.on("/screen", HTTP_GET, [](AsyncWebServerRequest *r){
      uint8_t* buf = display.getBuffer();
      String hex = "";
      for (int i = 0; i < 1024; i++) {
        if (buf[i] < 16) hex += "0";
        hex += String(buf[i], HEX);
      }
      r->send(200, "text/plain", hex);
    });

    server.begin();
    serverStarted = true;
    wifiPower = true;
  }

  // Display status on the OLED
  display.setCursor(0, 15); display.println("--- REMOTE ACTIVE ---");
  display.setCursor(0, 28); display.println("SSID: SWISS_ARMY");
  display.println("IP  : 192.168.4.1");
  display.print("Clients: "); display.println(WiFi.softAPgetStationNum());
  
  display.setCursor(0, 52); display.println("UP: BACK (Keep AP)");
  display.setCursor(0, 60); display.println("DN: STOP AP & EXIT");

  if (digitalRead(PIN_UP) == HIGH || v_up) {
    v_up = false; currentState = WIFI_MENU;
    while(digitalRead(PIN_UP) == HIGH); delay(200);
  }

  if (digitalRead(PIN_DOWN) == HIGH || v_down) { 
    v_down = false; WiFi.softAPdisconnect(true); server.end(); 
    serverStarted = false; wifiPower = false;
    currentState = WIFI_MENU; 
    while(digitalRead(PIN_DOWN) == HIGH); delay(300); 
  }
}

// --- 2. WIFI SCANNER ---
void handleWiFiScan() {
  display.setCursor(30, 35); display.print("Scanning..."); display.display();
  scannedCount = WiFi.scanNetworks();
  currentState = (scannedCount > 0) ? SCAN_WIFI : WIFI_MENU;
}

void drawWiFiList() {
  display.setCursor(0, 15); display.println("   PICK NETWORK:");
  int totalOptions = scannedCount + 1;
  int startIdx = (wifiListIdx >= 4) ? wifiListIdx - 3 : 0;
  
  for (int i = 0; i < 4; i++) {
    int cur = startIdx + i; if (cur >= totalOptions) break;
    int y = 25 + (i * 10);
    if (cur == wifiListIdx) { 
      display.fillRect(0, y-1, 128, 9, WHITE); display.setTextColor(BLACK); 
    } else display.setTextColor(WHITE);
    
    display.setCursor(5, y);
    if (cur == 0) display.print("<-- BACK");
    else display.print(WiFi.SSID(cur - 1).substring(0, 15));
  }
  display.setTextColor(WHITE);

  if (digitalRead(PIN_DOWN) == HIGH || v_down) { 
    v_down = false; wifiListIdx = (wifiListIdx + 1) % totalOptions; 
    while(digitalRead(PIN_DOWN) == HIGH); delay(200); 
  }
  if (digitalRead(PIN_UP) == HIGH || v_up) { 
    v_up = false; wifiListIdx = (wifiListIdx - 1 + totalOptions) % totalOptions; 
    while(digitalRead(PIN_UP) == HIGH); delay(200); 
  }
  if (digitalRead(PIN_SELECT) == HIGH || v_sel) { 
    v_sel = false;
    if (wifiListIdx == 0) currentState = WIFI_MENU;
    else {
      selectedSSID = WiFi.SSID(wifiListIdx - 1); 
      targetString = &password; keyboardLabel = "WIFI PASS"; 
      returnState = CONNECTING; currentState = ENTER_PASS; 
    }
    while(digitalRead(PIN_SELECT) == HIGH); delay(400); 
  }
}

#endif