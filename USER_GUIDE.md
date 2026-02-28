# ESP32 3-Button UI User Guide

This document explains how to navigate and use the ESP32 OLED interface using the 3 touch buttons.

## Button Layout
The system is controlled using 3 buttons connected to the following pins:
- **Up Button** (Pin 4)
- **Down Button** (Pin 18)
- **Select Button** (Pin 19)

---

## 1. Home Screen
The home screen displays system telemetry (CPU, Heap memory, Uptime).
- **Select Button (Short Press):** Opens the **Main Menu**.

---

## 2. Navigating Menus
From the Main Menu, WiFi Menu, or Network Selection list:
- **Down Button (Short Press):** Moves the selection cursor downwards. 
- **Up Button (Short Press):** Moves the selection cursor upwards.
- **Select Button (Short Press):** Confirms the selection and enters that sub-menu or executes the action.

---

## 3. Using the On-Screen Keyboard
The keyboard (used for entering the WiFi Password) is the most complex part of the interface to navigate with just 3 buttons. Please read these instructions carefully:

### Selecting Characters
- **Down Button (Short Press):** Moves the character selection to the right.
- **Up Button (Short Press):** Moves the character selection to the left.

### Changing Character Sets (Uppercase, Lowercase, Numbers/Symbols)
- **Up Button (LONG PRESS - Hold for more than 1 second):** Cycles between character sets. If you are on uppercase letters, holding the Up Button will switch to lowercase, then to numbers/symbols.

### Typing Characters
- **Select Button (Short Press):** Types the currently highlighted character on the screen.
  - *Note:* If you select the **`<`** symbol, it will act as a backspace.
  - *Note:* If you select the **`X`** symbol, it will Cancel and exit the keyboard without saving.

### Managing the Text (Backspace & Save)
If you made a mistake or finish typing, you can use the Select Button's hold features without navigating to a specific key:
- **Select Button (LONG PRESS - Hold for ~1 to 2.5 seconds):** Acts as a quick **Backspace**. Deletes the last typed character.
- **Select Button (EXTRA LONG PRESS - Hold for more than 2.5 seconds):** **Saves** the entered password and proceeds to connect to the WiFi network.

---

## 4. Status Pages & Placeholders
When viewing the WiFi connection status, Bluetooth (Locked), or IR Remote (Locked) screens:
- **Select Button (Short Press):** Goes back to the previous menu.
