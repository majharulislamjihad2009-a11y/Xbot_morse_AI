# XBOT Morse AI Chatbot (ESP8266 + OLED)

XBOT is a hardware-based Morse code AI chatbot built on ESP8266 with a 0.96-inch OLED display.  
The project combines Morse code input, real-time AI responses, and efficient memory management to create a compact, educational, and extensible embedded AI system.

This project is designed, implemented, and tested by the author.  
Idea and system design are original. AI is used as an assistant, not as a replacement for understanding.

---

## Project Motivation

XBOT was created to explore how modern AI systems can be integrated with extremely resource-constrained hardware.

Core goals:
- Use Morse code as a minimal and universal human–machine interface
- Run AI-assisted interactions on microcontrollers
- Build real engineering skills across hardware, firmware, networking, and system design

This is not a copy-paste project.  
It is a long-term learning and experimentation platform.

---

## Hardware Requirements

- ESP8266 (NodeMCU recommended)
- 0.96-inch OLED display (SSD1306, I2C)
- Push buttons:
  - Dot button
  - Dash button
  - Action button
  - Scroll up (Dash button)
  - Scroll down (Dot button)
  - Back (Action button)
  - Library
- Buzzer
  - Sound feedbacks 

---

## Wiring Diagram (Text Reference)

The following table describes the exact wiring used in this project.
All connections match the GPIO definitions in the source code.

## Wiring Connections:
 -----------------------------------------
 ESP8266 Pin  |  Component       | Pin
 -----------------------------------------
 3V3         -->  OLED Display   | VCC
 GND         -->  OLED Display   | GND
 D2 (GPIO4)  -->  OLED Display   | SCL
 D1 (GPIO5)  -->  OLED Display   | SDA
 -----------------------------------------
 D5 (GPIO14) -->  Button 1 (DOT) | Pin 1
 GND         -->  Button 1       | Pin 2
 -----------------------------------------
 D6 (GPIO12) -->  Button 2 (DASH)| Pin 1
 GND         -->  Button 2       | Pin 2
 -----------------------------------------
 D7 (GPIO13) -->  Button 3 (ACT) | Pin 1
 GND         -->  Button 3       | Pin 2
 -----------------------------------------
 D8 (GPIO15) -->  Buzzer         | Positive (+)
 GND         -->  Buzzer         | Negative (-)
 -----------------------------------------

### Note:
 All buttons use INPUT_PULLUP logic.
 Each button connects between the GPIO pin and GND.

---

## Key Features

### Morse-Based Input System
- Text input using dot (.) and dash (-) buttons
- Custom Morse commands for:
  - Send message to AI
  - Backspace
  - Space
  - Clear input
  - Toggle buzzer
  - Open Morse library

### AI Chatbot Integration
- Connects to OpenRouter API
- Uses DeepSeek chat model
- Configurable system personality (default: JARVIS-style assistant)
- Short, OLED-friendly responses

### Advanced Display System
- Smooth scrolling for long AI responses
- Page indicators and scroll bar
- Separate screens for:
  - WiFi connection
  - Morse input
  - Sending animation
  - Receiving state
  - AI response view
  - Error handling

### Built-in Morse Library
- On-device Morse reference
- Scrollable character list
- Learn Morse without external resources

### Memory-Optimized Design
- Designed for ESP8266 low-RAM limitations
- Controlled chat history
- Dynamic cleanup to reduce heap fragmentation
- Safe handling of large API responses

### Audio Feedback
- Professional buzzer tones for:
  - Dot and dash
  - Successful actions
  - Errors
  - Send confirmation
- Buzzer can be toggled on/off via Morse command

---

## Pin Configuration (Default)

- Dot button: GPIO 14  
- Dash button: GPIO 12  
- Action button: GPIO 13  
- Buzzer: GPIO 15  
- Scroll up: GPIO 16  
- Scroll down: GPIO 5  
- Back: GPIO 4  
- Library: GPIO 0  
- OLED I2C: SDA = GPIO 5, SCL = GPIO 4  

Pins can be changed from the source code if required.

---

## Software Requirements

- Arduino IDE
- ESP8266 board package
- Required libraries:
  - Adafruit GFX
  - Adafruit SSD1306
  - ArduinoJson
  - ESP8266WiFi
  - ESP8266HTTPClient

---

## Configuration

Before uploading the code, update the following fields:
---
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";
const char* API_KEY   = "YOUR_OPENROUTER_API_KEY";

API keys are not included in this repository for security reasons.

---

## User Guide

1. Power on the device  
2. Wait for WiFi connection  
3. Enter Morse code using dot and dash buttons  
4. Press ACTION to convert Morse into characters  
5. Use the SEND command to send the message to AI  
6. Read the AI response on the OLED display  
7. Scroll using the scroll buttons if the response is long  
8. Press BACK to return to the input screen  

---

## Customization

You can modify the following parts of the project:
- AI personality (system prompt)
- Morse command patterns
- Display layout and animations
- Memory limits and response size
- Hardware pin configuration

The code is structured to support easy experimentation and learning.

---

## Educational Value

This project demonstrates:
- Embedded system design using ESP8266
- Morse code decoding and command handling
- HTTPS API communication on microcontrollers
- JSON parsing with limited memory
- Real-time UI rendering on small OLED displays
- Memory optimization techniques for low-RAM devices

---

## Author

Developed by a Bangladeshi student (HSC 2027 batch) who focuses on building projects from real-life problems rather than tutorials.

Strong interests include:
- Embedded systems and low-level programming
- Physics-inspired problem solving and system thinking
- Human–machine interfaces using minimal hardware
- AI-assisted tools built with clear understanding, not blind automation

Most projects start from curiosity, experiments, and practical limitations, with an emphasis on learning how things work internally rather than just making them work.

---

## Disclaimer

This project is intended for educational and experimental purposes only.  
Users are responsible for their own API usage, costs, and hardware safety.

---

## Future Improvements

- Offline AI support  
- SD card logging  
- Multi-language profiles  
- Power optimization and battery support  




