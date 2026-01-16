#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// ========== DISPLAY CONFIGURATION ==========
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Font sizes
#define CHAR_WIDTH 6
#define CHAR_HEIGHT 8
#define MAX_LINES 8
#define CHARS_PER_LINE 20
#define MAX_RESPONSE_SIZE 3000

// ========== MEMORY OPTIMIZATION ==========
#define MAX_HISTORY_MESSAGES 2
#define MAX_HISTORY_SIZE 1500

// ========== HARDWARE PINS ==========
#define DOT_PIN 14
#define DASH_PIN 12
#define ACTION_PIN 13
#define BUZZER_PIN 15

// ========== SCROLLING PINS ==========
#define SCROLL_UP_PIN 16
#define SCROLL_DOWN_PIN 5
#define BACK_PIN 4

// ========== LIBRARY MENU PINS ==========
#define LIBRARY_PIN 0

// ========== TIMING ==========
#define DOT_DURATION 80
#define DASH_DURATION 240
#define DEBOUNCE_DELAY 50
#define LONG_PRESS_TIME 1000
#define API_TIMEOUT 40000
#define ANIMATION_DELAY 150

// ========== MORSE COMMANDS ==========
const String CMD_SEND = ".-.-..";
const String CMD_BACKSPACE = "........";
const String CMD_SPACE = ".-.-";
const String CMD_CLEAR = "----";
const String CMD_BUZZER_TOGGLE = "-.....-";
const String CMD_LIBRARY = ".-....";  // Fixed library command

// ========== CONFIGURATION ==========
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";
const char* API_KEY = "YOUR_OPENROUTER_API_KEY";
const char* API_URL = "https://openrouter.ai/api/v1/chat/completions";

// ========== SYSTEM STATES ==========
enum SystemState {
  STATE_WIFI_CONNECTING,
  STATE_READY,
  STATE_MORSE_INPUT,
  STATE_CONVERTING,
  STATE_SENDING,
  STATE_RECEIVING,
  STATE_AI_RESPONSE,
  STATE_ERROR,
  STATE_LIBRARY_MENU
};

// ========== GLOBAL VARIABLES ==========
SystemState currentState = STATE_WIFI_CONNECTING;
String morseBuffer = "";
String textBuffer = "";
String fullAIResponse = "";
String displayResponse = "";
unsigned long lastApiCall = 0;
unsigned long buttonPressTime = 0;

bool buzzerEnabled = true;
bool dotPressed = false;
bool dashPressed = false;
bool actionPressed = false;
bool actionLongPressed = false;
bool scrollUpPressed = false;
bool scrollDownPressed = false;
bool backPressed = false;
bool libraryPressed = false;

int scrollOffset = 0;
int totalResponseLines = 0;
String responseLines[80];

// Library variables - ONE CHARACTER PER LINE WITH CURSOR
int libraryCurrentChar = 0;        // Current selected character index (0 to MORSE_COUNT-1)
int libraryCursorLine = 0;         // Cursor position on screen (0 to 5)
int libraryTopIndex = 0;           // Top visible character index

// Animation variables for sending screen
unsigned long lastSendAnimUpdate = 0;
int sendAnimFrame = 0;

// ========== CHAT HISTORY VARIABLES ==========
struct ChatMessage {
  String role;
  String content;
  unsigned long timestamp;
};

ChatMessage chatHistory[MAX_HISTORY_MESSAGES];
int chatHistoryCount = 0;
String historyBuffer = "";

// ========== MORSE TABLE WITH ALL CHARACTERS ==========
struct MorseChar {
  char symbol;
  String pattern;
  String description;  // Added for better display
};

MorseChar morseMap[] = {
  // Letters
  { 'A', ".-", "A" }, { 'B', "-...", "B" }, { 'C', "-.-.", "C" }, { 'D', "-..", "D" }, 
  { 'E', ".", "E" }, { 'F', "..-.", "F" }, { 'G', "--.", "G" }, { 'H', "....", "H" }, 
  { 'I', "..", "I" }, { 'J', ".---", "J" }, { 'K', "-.-", "K" }, { 'L', ".-..", "L" }, 
  { 'M', "--", "M" }, { 'N', "-.", "N" }, { 'O', "---", "O" }, { 'P', ".--.", "P" }, 
  { 'Q', "--.-", "Q" }, { 'R', ".-.", "R" }, { 'S', "...", "S" }, { 'T', "-", "T" }, 
  { 'U', "..-", "U" }, { 'V', "...-", "V" }, { 'W', ".--", "W" }, { 'X', "-..-", "X" }, 
  { 'Y', "-.--", "Y" }, { 'Z', "--..", "Z" },
  { '0', "-----", "0" }, { '1', ".----", "1" }, { '2', "..---", "2" }, { '3', "...--", "3" }, 
  { '4', "....-", "4" }, { '5', ".....", "5" }, { '6', "-....", "6" }, { '7', "--...", "7" }, 
  { '8', "---..", "8" }, { '9', "----.", "9" },
  { '.', ".-.-.-", "Period" }, { ',', "--..--", "Comma" }, { '?', "..--..", "Question" },
  { '!', "-.-.--", "Exclamation" }, { '/', "-..-.", "Slash" }, { '(', "-.--.", "Open Paren" },
  { ')', "-.--.-", "Close Paren" }, { '&', ".-...", "Ampersand" }, { ':', "---...", "Colon" },
  { ';', "-.-.-.", "Semicolon" }, { '=', "-...-", "Equals" }, { '+', ".-.-.", "Plus" },
  { '-', "-....-", "Hyphen" }, { '_', "..--.-", "Underscore" }, { '"', ".-..-.", "Quote" },
  { '$', "...-..-", "Dollar" }, { '@', ".--.-.", "At Sign" }, { '\'', ".----.", "Apostrophe" },
  { ' ', "/", "Space" }, { '\\', "-..-.", "Backslash" }, { '[', "-.--.", "Bracket Open" },
  { ']', "-.--.-", "Bracket Close" }, { '{', "-.--.", "Brace Open" }, { '}', "-.--.-", "Brace Close" },
  { '<', "-..-", "Less Than" }, { '>', ".-.--", "Greater Than" }, { '%', "-...-.-", "Percent" },
  { '^', "..--", "Caret" }, { '*', "-..-", "Asterisk" }, { '~', ".-.-", "Tilde" }
};

const int MORSE_COUNT = sizeof(morseMap) / sizeof(morseMap[0]);

// ========== FUNCTION DECLARATIONS ==========
void initSystem();
void connectWiFi();
void handleButtons();
void processDot();
void processDash();
void processAction();
void convertMorseToChar();
void sendToAI();
void clearInput();
void prepareResponseForDisplay();
void updateDisplay();
void drawWiFiConnecting();
void drawReadyScreen();
void drawMorseInput();
void drawConverting();
void drawSending();
void drawReceiving();
void drawAIResponse();
void drawErrorScreen(String errorMsg);
void drawLibraryMenu();
void playBeep(int freq, int duration);
void playSuccessTone();
void playErrorTone();
void playDotTone();
void playDashTone();
void playSendTone();
String makeAPIRequest(String query);
String extractAndCleanResponse(String payload);
void handleScrollingButtons();    
void handleLibraryButton();       
void initScrollingPins();         
void initLibraryPin();
void addToChatHistory(String role, String content);
void updateHistoryBuffer();
void clearChatHistory();
String getChatHistoryForAPI();
void displayChatHistoryStatus();
void toggleBuzzer();
void freeMemory();

// ========================================================================
// SETUP FUNCTION
// ========================================================================
void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println("\n================================================");
  Serial.println("        MORSE AI CHATBOT - ESP8266");
  Serial.println("================================================");

  initSystem();

  clearChatHistory();

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(25, 20);
  display.print(" XBOT");
  display.setTextSize(1);
  display.setCursor(90, 35);
  display.print("AI");
  display.display();

  playDotTone();
  delay(150);
  playDashTone();
  delay(800);

  connectWiFi();
}

// ========================================================================
// MAIN LOOP WITH MEMORY OPTIMIZATION
// ========================================================================
void loop() {
  handleButtons();
  
  // Update animation in sending state
  if (currentState == STATE_SENDING) {
    unsigned long now = millis();
    if (now - lastSendAnimUpdate > 150) {
      updateDisplay();
    }
  }
  
  // Optional: Memory check every 10 seconds
  static unsigned long lastMemoryCheck = 0;
  unsigned long now = millis();
  if (now - lastMemoryCheck > 10000) {
    lastMemoryCheck = now;
    if (ESP.getFreeHeap() < 3000) {
      Serial.println("[MEMORY WARNING] Periodic check: Low memory!");
      freeMemory();
    }
  }
  
  updateDisplay();
  delay(20);
}

// ========================================================================
// CHAT HISTORY FUNCTIONS - OPTIMIZED FOR SINGLE EXCHANGE
// ========================================================================
void addToChatHistory(String role, String content) {
  // Free old memory first
  freeMemory();
  
  // Keep only the latest exchange (1 user + 1 assistant message)
  if (role == "user") {
    // Clear everything when new user message arrives
    chatHistoryCount = 0;
    historyBuffer = "";
    
    // Add user message
    if (chatHistoryCount < MAX_HISTORY_MESSAGES) {
      chatHistory[chatHistoryCount].role = role;
      chatHistory[chatHistoryCount].content = content;
      chatHistory[chatHistoryCount].timestamp = millis();
      chatHistoryCount++;
    }
  } 
  else if (role == "assistant") {
    // Add assistant response only if user message exists
    if (chatHistoryCount == 1) { // 1 means only user message exists
      chatHistory[chatHistoryCount].role = role;
      chatHistory[chatHistoryCount].content = content;
      chatHistory[chatHistoryCount].timestamp = millis();
      chatHistoryCount++;
    }
  }
  
  updateHistoryBuffer();

  Serial.print("[HISTORY] Added ");
  Serial.print(role);
  Serial.print(" message. Total messages: ");
  Serial.println(chatHistoryCount);
  
  // Free memory again
  freeMemory();
}

void updateHistoryBuffer() {
  // Clear old buffer first
  historyBuffer = "";
  
  // Recreate buffer
  for (int i = 0; i < chatHistoryCount; i++) {
    String formattedMsg = chatHistory[i].role + ": " + chatHistory[i].content + "\n";
    historyBuffer += formattedMsg;
  }
}

void clearChatHistory() {
  // Clear all strings
  for (int i = 0; i < MAX_HISTORY_MESSAGES; i++) {
    chatHistory[i].role = String();
    chatHistory[i].content = String();
    chatHistory[i].timestamp = 0;
  }
  
  chatHistoryCount = 0;
  historyBuffer = String();
  
  // Force garbage collection
  ESP.wdtFeed();
  yield();
  
  Serial.println("[HISTORY] Chat history cleared");
  Serial.printf("[MEMORY] Free heap after clearing: %d bytes\n", ESP.getFreeHeap());
}

String getChatHistoryForAPI() {
  String apiHistory = "";
  
  // Send only if complete exchange exists (2 messages)
  if (chatHistoryCount == 2) {
    for (int i = 0; i < chatHistoryCount; i++) {
      // JSON escaping
      String escapedContent = chatHistory[i].content;
      escapedContent.replace("\"", "\\\"");
      escapedContent.replace("\\", "\\\\");
      escapedContent.replace("\n", "\\n");
      escapedContent.replace("\r", "\\r");
      escapedContent.replace("\t", "\\t");
      
      String jsonMsg = "{\"role\":\"" + chatHistory[i].role + "\",\"content\":\"" + escapedContent + "\"}";
      
      if (apiHistory.length() > 0) {
        apiHistory += ",";
      }
      apiHistory += jsonMsg;
    }
  }
  
  return apiHistory;
}

// ========================================================================
// SEND TONE FUNCTION
// ========================================================================
void playSendTone() {
  if (!buzzerEnabled) return;
  
  playBeep(800, 80);
  delay(40);
  playBeep(1000, 80);
  delay(40);
  playBeep(1200, 120);
  Serial.println("[SOUND] Send tone played");
}

// ========================================================================
// LIBRARY MENU FUNCTIONS - FIXED SCROLLING
// ========================================================================
void initLibraryPin() {
  pinMode(LIBRARY_PIN, INPUT_PULLUP);
  Serial.println("[LIBRARY] Library button initialized on D3");
}

void handleLibraryButton() {
  static unsigned long lastLibraryDebounce = 0;
  unsigned long now = millis();

  if (now - lastLibraryDebounce < DEBOUNCE_DELAY) return;
  lastLibraryDebounce = now;

  if (digitalRead(LIBRARY_PIN) == LOW && !libraryPressed) {
    libraryPressed = true;
    
    if (currentState == STATE_READY || currentState == STATE_MORSE_INPUT) {
      currentState = STATE_LIBRARY_MENU;
      libraryCurrentChar = 0;
      libraryCursorLine = 0;
      libraryTopIndex = 0;
      playBeep(800, 100);
      delay(50);
      playBeep(1000, 100);
      Serial.println("[LIBRARY] Opening library menu");
    }
  } else if (digitalRead(LIBRARY_PIN) == HIGH) {
    libraryPressed = false;
  }
}

void drawLibraryMenu() {
  display.clearDisplay();
  
  display.setTextSize(1);
  display.setTextColor(WHITE);
  
  // Header
  display.setCursor(0, 0);
  display.print("MORSE LIBRARY");
  
  // Position indicator
  display.setCursor(90, 0);
  display.print(libraryCurrentChar + 1);
  display.print("/");
  display.print(MORSE_COUNT);
  
  // Calculate which characters to show (6 lines total)
  int startIndex = libraryTopIndex;
  
  // Draw 6 characters (ONE CHARACTER PER LINE)
  for (int i = 0; i < 6; i++) {
    int charIndex = startIndex + i;
    if (charIndex >= MORSE_COUNT) break;
    
    int yPos = 10 + (i * 9);
    
    // Highlight current line
    if (charIndex == libraryCurrentChar) {
      display.fillRect(0, yPos - 1, 128, 9, WHITE);
      display.setTextColor(BLACK);
    } else {
      display.setTextColor(WHITE);
    }
    
    // Display character and pattern
    display.setCursor(5, yPos);
    display.print(morseMap[charIndex].symbol);
    display.print("  :  ");
    display.print(morseMap[charIndex].pattern);
    
    // Reset text color if highlighted
    if (charIndex == libraryCurrentChar) {
      display.setTextColor(WHITE);
    }
  }
  
}

// ========================================================================
// SYSTEM INITIALIZATION
// ========================================================================
void initSystem() {
  Serial.println("[SYSTEM] Initializing hardware...");

  Wire.begin(5, 4);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("[ERROR] OLED initialization failed!");
    while (1);
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextWrap(false);

  pinMode(DOT_PIN, INPUT_PULLUP);
  pinMode(DASH_PIN, INPUT_PULLUP);
  pinMode(ACTION_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  initScrollingPins();
  initLibraryPin();

  Serial.println("[SYSTEM] Hardware ready");
  Serial.println("[BUZZER] Default state: ENABLED");
}

// ========================================================================
// INITIALIZE SCROLLING PINS
// ========================================================================
void initScrollingPins() {
  pinMode(SCROLL_UP_PIN, INPUT_PULLUP);
  pinMode(SCROLL_DOWN_PIN, INPUT_PULLUP);
  pinMode(BACK_PIN, INPUT_PULLUP);
  Serial.println("[SCROLL] Scrolling buttons initialized");
}

// ========================================================================
// WIFI CONNECTION
// ========================================================================
void connectWiFi() {
  Serial.print("[WIFI] Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < 25) {
    delay(500);
    attempts++;
    if (attempts % 2 == 0) {
      Serial.print(".");
      updateDisplay();
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WIFI] Connected successfully");
    Serial.print("[WIFI] IP Address: ");
    Serial.println(WiFi.localIP());
    currentState = STATE_READY;
    playSuccessTone();
  } else {
    Serial.println("\n[ERROR] WiFi connection failed");
    currentState = STATE_ERROR;
  }
}

// ========================================================================
// HANDLE SCROLLING BUTTONS - FIXED FOR ALL STATES
// ========================================================================
void handleScrollingButtons() {
  static unsigned long lastScrollDebounce = 0;
  unsigned long now = millis();

  if (now - lastScrollDebounce < DEBOUNCE_DELAY) return;
  lastScrollDebounce = now;

  // Handle scroll up (D5)
  if (digitalRead(SCROLL_UP_PIN) == LOW && !scrollUpPressed) {
    scrollUpPressed = true;
    
    if (currentState == STATE_AI_RESPONSE) {
      if (scrollOffset > 0) {
        scrollOffset--;
        playBeep(600, 30);
      }
    } else if (currentState == STATE_LIBRARY_MENU) {
      // Use DOT for scrolling up in library
      if (libraryCurrentChar > 0) {
        libraryCurrentChar--;
        // Adjust visible window
        if (libraryCurrentChar < libraryTopIndex) {
          libraryTopIndex = libraryCurrentChar;
        }
        playBeep(600, 30);
      }
    }
  } else if (digitalRead(SCROLL_UP_PIN) == HIGH) {
    scrollUpPressed = false;
  }

  // Handle scroll down (D6)
  if (digitalRead(SCROLL_DOWN_PIN) == LOW && !scrollDownPressed) {
    scrollDownPressed = true;
    
    if (currentState == STATE_AI_RESPONSE) {
      if (scrollOffset < totalResponseLines - 6 && totalResponseLines > 6) {
        scrollOffset++;
        playBeep(600, 30);
      }
    } else if (currentState == STATE_LIBRARY_MENU) {
      // Use DASH for scrolling down in library
      if (libraryCurrentChar < MORSE_COUNT - 1) {
        libraryCurrentChar++;
        // Adjust visible window
        if (libraryCurrentChar >= libraryTopIndex + 6) {
          libraryTopIndex = libraryCurrentChar - 5;
        }
        playBeep(600, 30);
      }
    }
  } else if (digitalRead(SCROLL_DOWN_PIN) == HIGH) {
    scrollDownPressed = false;
  }

  // Handle back to morse input (D7)
  if (digitalRead(BACK_PIN) == LOW && !backPressed) {
    backPressed = true;
    delay(DEBOUNCE_DELAY);
    
    if (currentState == STATE_LIBRARY_MENU) {
      currentState = STATE_READY;
      playDashTone();
      Serial.println("[LIBRARY] Exiting library menu");
    } else if (currentState == STATE_AI_RESPONSE) {
      currentState = STATE_READY;
      scrollOffset = 0;
      playDashTone();
    }
  } else if (digitalRead(BACK_PIN) == HIGH) {
    backPressed = false;
  }
}

// ========================================================================
// BUTTON HANDLING - FIXED FOR LIBRARY SCROLLING
// ========================================================================
void handleButtons() {
  static unsigned long lastDebounce = 0;
  unsigned long now = millis();

  if (now - lastDebounce < DEBOUNCE_DELAY) return;
  lastDebounce = now;

  handleLibraryButton();
  handleScrollingButtons();

  if (currentState == STATE_ERROR) {
    if (digitalRead(ACTION_PIN) == LOW && !actionPressed) {
      actionPressed = true;
      delay(DEBOUNCE_DELAY);
      currentState = STATE_READY;
      textBuffer = "";
      morseBuffer = "";
      playDotTone();
    } else if (digitalRead(ACTION_PIN) == HIGH) {
      actionPressed = false;
    }
    return;
  }

  if (currentState == STATE_LIBRARY_MENU) {
    // Handle DOT for scrolling UP in library
    if (digitalRead(DOT_PIN) == LOW && !dotPressed) {
      dotPressed = true;
      if (libraryCurrentChar > 0) {
        libraryCurrentChar--;
        // Adjust visible window
        if (libraryCurrentChar < libraryTopIndex) {
          libraryTopIndex = libraryCurrentChar;
        }
        playBeep(600, 30);
      }
    } else if (digitalRead(DOT_PIN) == HIGH) {
      dotPressed = false;
    }

    // Handle DASH for scrolling DOWN in library
    if (digitalRead(DASH_PIN) == LOW && !dashPressed) {
      dashPressed = true;
      if (libraryCurrentChar < MORSE_COUNT - 1) {
        libraryCurrentChar++;
        // Adjust visible window
        if (libraryCurrentChar >= libraryTopIndex + 6) {
          libraryTopIndex = libraryCurrentChar - 5;
        }
        playBeep(600, 30);
      }
    } else if (digitalRead(DASH_PIN) == HIGH) {
      dashPressed = false;
    }

    // Action button to exit library
    if (digitalRead(ACTION_PIN) == LOW && !actionPressed) {
      actionPressed = true;
      delay(DEBOUNCE_DELAY);
      currentState = STATE_READY;
      playDashTone();
      Serial.println("[LIBRARY] Exiting library menu");
    } else if (digitalRead(ACTION_PIN) == HIGH) {
      actionPressed = false;
    }
    return;
  }

  if (currentState == STATE_AI_RESPONSE) {
    if (digitalRead(DOT_PIN) == LOW && !dotPressed) {
      dotPressed = true;
      if (scrollOffset > 0) {
        scrollOffset--;
        playBeep(600, 30);
      }
    } else if (digitalRead(DOT_PIN) == HIGH) {
      dotPressed = false;
    }

    if (digitalRead(DASH_PIN) == LOW && !dashPressed) {
      dashPressed = true;
      if (scrollOffset < totalResponseLines - 6 && totalResponseLines > 6) {
        scrollOffset++;
        playBeep(600, 30);
      }
    } else if (digitalRead(DASH_PIN) == HIGH) {
      dashPressed = false;
    }

    if (digitalRead(ACTION_PIN) == LOW && !actionPressed) {
      actionPressed = true;
      delay(DEBOUNCE_DELAY);
      currentState = STATE_READY;
      scrollOffset = 0;
      playDashTone();
    } else if (digitalRead(ACTION_PIN) == HIGH) {
      actionPressed = false;
    }
    return;
  }

  // Normal input mode - Dot button
  if (digitalRead(DOT_PIN) == LOW && !dotPressed) {
    dotPressed = true;
    morseBuffer += ".";
    currentState = STATE_MORSE_INPUT;
    playDotTone();
    Serial.print(".");
  } else if (digitalRead(DOT_PIN) == HIGH) {
    dotPressed = false;
  }

  // Normal input mode - Dash button
  if (digitalRead(DASH_PIN) == LOW && !dashPressed) {
    dashPressed = true;
    morseBuffer += "-";
    currentState = STATE_MORSE_INPUT;
    playDashTone();
    Serial.print("-");
  } else if (digitalRead(DASH_PIN) == HIGH) {
    dashPressed = false;
  }

  // Action button handling
  if (digitalRead(ACTION_PIN) == LOW && !actionPressed) {
    actionPressed = true;
    buttonPressTime = millis();

    // Detect long press
    while (digitalRead(ACTION_PIN) == LOW) {
      if (millis() - buttonPressTime > LONG_PRESS_TIME) {
        actionLongPressed = true;
        playBeep(500, 200);
        break;
      }
      delay(10);
    }

    delay(DEBOUNCE_DELAY);

    if (actionLongPressed) {
      // Check if very long press to clear history (press longer than 3 seconds)
      if (millis() - buttonPressTime > 3000) {
        clearChatHistory();
        playBeep(300, 100);
        delay(100);
        playBeep(300, 100);
        delay(100);
        playBeep(300, 100);
      } else {
        clearInput();
      }
      actionLongPressed = false;
    } else {
      if (currentState == STATE_MORSE_INPUT && morseBuffer.length() > 0) {
        currentState = STATE_CONVERTING;
        convertMorseToChar();
      } else if (currentState == STATE_READY && textBuffer.length() > 0) {
        sendToAI();
      }
    }

  } else if (digitalRead(ACTION_PIN) == HIGH) {
    actionPressed = false;
  }
}

// ========================================================================
// MORSE PROCESSING
// ========================================================================
void convertMorseToChar() {
  Serial.print("\n[MORSE] ");
  Serial.print(morseBuffer);
  Serial.print(" -> ");

  if (morseBuffer == CMD_SEND) {
    Serial.println("SEND COMMAND");
    if (textBuffer.length() > 0) {
      sendToAI();
    } else {
      Serial.println("[INFO] No text to send");
      playBeep(300, 200);
    }
    morseBuffer = "";
    currentState = STATE_READY;
    return;
  }

  if (morseBuffer == CMD_BACKSPACE) {
    if (textBuffer.length() > 0) {
      textBuffer.remove(textBuffer.length() - 1);
      Serial.println("BACKSPACE");
      playBeep(400, 100);
    }
    morseBuffer = "";
    currentState = STATE_READY;
    return;
  }

  if (morseBuffer == CMD_SPACE) {
    textBuffer += " ";
    Serial.println("SPACE");
    morseBuffer = "";
    currentState = STATE_READY;
    return;
  }

  if (morseBuffer == CMD_CLEAR) {
    clearInput();
    morseBuffer = "";
    currentState = STATE_READY;
    return;
  }

  if (morseBuffer == CMD_BUZZER_TOGGLE) {
    Serial.println("BUZZER TOGGLE COMMAND");
    toggleBuzzer();
    morseBuffer = "";
    currentState = STATE_READY;
    return;
  }

  if (morseBuffer == CMD_LIBRARY) {
    Serial.println("LIBRARY COMMAND");
    currentState = STATE_LIBRARY_MENU;
    libraryCurrentChar = 0;
    libraryTopIndex = 0;
    playBeep(800, 100);
    delay(50);
    playBeep(1000, 100);
    morseBuffer = "";
    return;
  }

  bool found = false;
  for (int i = 0; i < MORSE_COUNT; i++) {
    if (morseMap[i].pattern == morseBuffer) {
      char decoded = morseMap[i].symbol;
      textBuffer += decoded;
      Serial.println(decoded);
      found = true;
      playBeep(800, 80);
      delay(30);
      playBeep(1000, 80);
      break;
    }
  }

  if (!found) {
    Serial.println("UNKNOWN CHARACTER");
    playErrorTone();
  }

  morseBuffer = "";
  currentState = STATE_READY;
}

// ========================================================================
// ANIMATED SENDING SCREEN - WORKING VERSION
// ========================================================================
void drawSending() {
  unsigned long now = millis();
  
  // Update animation every 150ms
  if (now - lastSendAnimUpdate > 150) {
    sendAnimFrame = (sendAnimFrame + 1) % 12;
    lastSendAnimUpdate = now;
  }
  
  display.clearDisplay();
  
  // Title
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("SENDING TO AI");
  
  // Animated arrow with dots
  display.setCursor(30, 15);
  display.print(">>> SENDING >>>");
  
  // Animated dots moving right
  for (int i = 0; i < 8; i++) {
    int xPos = 20 + (i * 12);
    int yPos = 25;
    int activeDot = (sendAnimFrame + i) % 8;
    
    if (i == activeDot) {
      display.fillCircle(xPos, yPos, 2, WHITE);
    } else {
      display.drawCircle(xPos, yPos, 1, WHITE);
    }
  }
  
  // Message preview in a box
  display.drawRect(0, 32, 128, 20, WHITE);
  
  String displayMsg = textBuffer;
  if (displayMsg.length() > 18) {
    displayMsg = displayMsg.substring(0, 15) + "...";
  }
  
  display.setCursor(4, 39);
  display.print("Msg: ");
  display.print(displayMsg);
  
  // Progress bar at bottom with animation
  display.drawRect(0, 54, 128, 8, WHITE);
  
  // Animated progress bar
  int barWidth = map(sendAnimFrame, 0, 11, 5, 126);
  display.fillRect(1, 55, barWidth, 6, WHITE);
  
  // Status text with pulsing dots
  display.setCursor(0, 63);
  display.print("Sending");
  
  // Pulsing dots animation
  int dotCount = (sendAnimFrame % 4) + 1;
  for (int i = 0; i < dotCount; i++) {
    display.print(".");
  }
}

// ========================================================================
// ENHANCED SEND TO AI WITH MEMORY OPTIMIZATION
// ========================================================================
void sendToAI() {
  if (textBuffer.length() == 0) {
    Serial.println("[ERROR] Message is empty");
    playErrorTone();
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[ERROR] WiFi disconnected");
    currentState = STATE_ERROR;
    playErrorTone();
    return;
  }

  Serial.print("\n[AI] Sending message: ");
  Serial.println(textBuffer);
  
  // Current memory status
  Serial.printf("[MEMORY] Before sending: %d bytes\n", ESP.getFreeHeap());

  // Play send tone
  playSendTone();
  
  // Reset animation variables
  sendAnimFrame = 0;
  lastSendAnimUpdate = millis();
  
  // Set state to show animation
  currentState = STATE_SENDING;
  
  // Update display immediately to show animation
  updateDisplay();
  delay(150); // Give some time to see animation
  
  lastApiCall = millis();
  
  // Memory cleanup
  freeMemory();

  // Now make API request
  String response = makeAPIRequest(textBuffer);

  if (response.length() > 0 && !response.startsWith("ERROR:")) {
    // Add user message to history
    addToChatHistory("user", textBuffer);
    
    // Add assistant response to history
    addToChatHistory("assistant", response);

    fullAIResponse = response;
    displayResponse = response;
    prepareResponseForDisplay();
    currentState = STATE_AI_RESPONSE;
    scrollOffset = 0;

    playSuccessTone();

    Serial.print("[AI] Response received (");
    Serial.print(response.length());
    Serial.println(" chars)");

    // Final memory status
    Serial.printf("[MEMORY] After response: %d bytes\n", ESP.getFreeHeap());
    
    // Final memory cleanup
    freeMemory();
  } else {
    Serial.print("[AI] Error response: ");
    Serial.println(response);
    currentState = STATE_ERROR;
    playErrorTone();
    
    // Memory cleanup in error state
    freeMemory();
  }
}
// ========================================================================
// UPDATE DISPLAY FUNCTION
// ========================================================================
void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  switch (currentState) {
    case STATE_WIFI_CONNECTING:
      drawWiFiConnecting();
      break;
    case STATE_READY:
      drawReadyScreen();
      break;
    case STATE_MORSE_INPUT:
      drawMorseInput();
      break;
    case STATE_CONVERTING:
      drawConverting();
      break;
    case STATE_SENDING:
      drawSending();
      break;
    case STATE_RECEIVING:
      drawReceiving();
      break;
    case STATE_AI_RESPONSE:
      drawAIResponse();
      break;
    case STATE_ERROR:
      drawErrorScreen("System Error");
      break;
    case STATE_LIBRARY_MENU:
      drawLibraryMenu();
      break;
  }

  display.display();
}

// ========================================================================
// BUZZER CONTROL FUNCTIONS
// ========================================================================
void toggleBuzzer() {
  buzzerEnabled = !buzzerEnabled;

  if (buzzerEnabled) {
    Serial.println("[BUZZER] Buzzer enabled");
    playBeep(1000, 100);
    delay(50);
    playBeep(1200, 100);
  } else {
    Serial.println("[BUZZER] Buzzer disabled");
    playBeep(600, 100);
    delay(50);
    playBeep(400, 100);
  }
}

// ========================================================================
// ADVANCED MEMORY MANAGEMENT - ESP8266 OPTIMIZED
// ========================================================================
void freeMemory() {
  // String memory optimization - only reserve, don't cut
  morseBuffer.reserve(20);
  textBuffer.reserve(200);
  
  // Reserve memory only for UI display
  displayResponse.reserve(800);
  
  // Cut only input text (user input)
  if (textBuffer.length() > 150) {
    textBuffer = textBuffer.substring(0, 150);
    textBuffer += "...";
  }
  
  // Don't cut AI response - keep full response
  // Work only for display optimization
  
  // Clear response lines if not needed
  if (currentState != STATE_AI_RESPONSE && currentState != STATE_RECEIVING) {
    for (int i = 20; i < 80; i++) {
      responseLines[i] = String();
    }
  }
  
  // ESP8266 garbage collection
  ESP.wdtFeed();  // Watchdog feed
  yield();        // Allow background tasks
  
  // Try to reduce heap fragmentation
  if (ESP.getFreeHeap() < 3000) {
    Serial.println("[MEMORY WARNING] Low memory! Forcing cleanup...");
    
    // Hard cleanup - only clear unnecessary strings
    if (currentState != STATE_AI_RESPONSE) {
      displayResponse = "";
    }
    
    morseBuffer = "";
    
    // Reserve to reduce memory allocation
    morseBuffer.reserve(10);
    textBuffer.reserve(150);
    displayResponse.reserve(600);
    
    // Chat history cleanup (if taking too much memory)
    if (chatHistoryCount > 0 && ESP.getFreeHeap() < 2000) {
      clearChatHistory();
    }
  }
  
  Serial.printf("[MEMORY] Free heap: %d bytes, Fragmentation: %d%%\n", 
                ESP.getFreeHeap(), ESP.getHeapFragmentation());
}

// ========================================================================
// DISPLAY FUNCTIONS
// ========================================================================
void drawWiFiConnecting() {
  static int counter = 0;
  counter = (counter + 1) % 4;

  display.setCursor(0, 0);
  display.print("CONNECTING");

  display.setCursor(0, 12);
  display.print("Network:");
  display.setCursor(0, 20);
  display.print(WIFI_SSID);

  display.setCursor(0, 36);
  display.print("Status:");
  display.setCursor(0, 44);
  display.print("Connecting");
  for (int i = 0; i < counter; i++) {
    display.print(".");
  }
}

void drawReadyScreen() {
  display.setCursor(0, 0);
  display.print("XBOT");
  display.setCursor(90, 0);
  display.print("H:");
  display.print(chatHistoryCount / 2);

  display.setCursor(0, 12);
  if (textBuffer.length() == 0) {
    display.print("Enter Morse code");
  } else {
    String displayText = textBuffer;
    if (displayText.length() > CHARS_PER_LINE) {
      displayText = "..." + displayText.substring(displayText.length() - CHARS_PER_LINE + 3);
    }
    display.print(displayText);
  }

  display.setCursor(0, 24);
  display.print("Input: ");
  display.print(morseBuffer);

  display.setCursor(0, 36);
  display.print("Length: ");
  display.print(textBuffer.length());
  display.print(" chars");

  display.setCursor(0, 56);
  if (chatHistoryCount > 0) {
    display.print("Chats:");
    display.print(chatHistoryCount / 2);
    display.print("1");
  }
}

void drawMorseInput() {
  display.setCursor(0, 0);
  display.print("ENTERING MORSE");

  display.setCursor(0, 12);
  if (textBuffer.length() > CHARS_PER_LINE) {
    String shortText = textBuffer.substring(textBuffer.length() - CHARS_PER_LINE + 3);
    display.print("...");
    display.print(shortText);
  } else {
    display.print(textBuffer);
  }

  display.setCursor(0, 24);
  display.print("Code: ");
  display.print(morseBuffer);

  for (int i = 0; i < MORSE_COUNT; i++) {
    if (morseMap[i].pattern == morseBuffer) {
      display.setCursor(90, 24);
      display.print("> ");
      display.print(morseMap[i].symbol);
      break;
    }
  }

  display.setCursor(0, 36);
  display.print("Press ACTION to");
  display.setCursor(0, 44);
  display.print("convert character");
}

void drawConverting() {
  display.setCursor(0, 0);
  display.print("CONVERTING");

  display.setCursor(0, 20);
  display.print("Processing");

  static int dots = 0;
  dots = (dots + 1) % 4;
  for (int i = 0; i < dots; i++) {
    display.print(".");
  }
}

void drawReceiving() {
  display.setCursor(0, 0);
  display.print("RECEIVING");

  display.setCursor(0, 12);
  display.print("AI is generating");
  display.setCursor(0, 20);
  display.print("response");

  static int anim = 0;
  anim = (anim + 1) % 8;

  display.setCursor(0, 36);
  display.print("[");
  for (int i = 0; i < 20; i++) {
    if (i < (anim % 21)) {
      display.print("=");
    } else {
      display.print(" ");
    }
  }
  display.print("]");

  display.setCursor(0, 48);
  display.print("Please wait");

  static int dots = 0;
  dots = (dots + 1) % 4;
  for (int i = 0; i < dots; i++) {
    display.print(".");
  }
}

void drawAIResponse() {
  display.clearDisplay();
  
  display.setCursor(0, 0);
  display.print("XBOT");
  
  // Debug info
  display.setCursor(90, 0);
  display.print("L:");
  display.print(totalResponseLines);
  
  int availableLines = 6; // 64 pixels / 9 = 7, but 1 line for header
  int startLine = scrollOffset;
  int linesDrawn = 0;
  
  // Draw only non-empty lines
  for (int i = 0; i < availableLines; i++) {
    int currentLine = i + startLine;
    if (currentLine >= totalResponseLines) {
      break;
    }
    
    String line = responseLines[currentLine];
    line.trim();
    
    // Skip completely empty lines
    if (line.length() == 0) {
      continue;
    }
    
    // Truncate line if too long
    if (line.length() > CHARS_PER_LINE) {
      line = line.substring(0, CHARS_PER_LINE);
    }
    
    int yPosition = 10 + (linesDrawn * 9);
    display.setCursor(0, yPosition);
    display.print(line);
    linesDrawn++;
    
    // Don't draw more than available lines
    if (linesDrawn >= availableLines) {
      break;
    }
  }
  
  // If no lines were drawn but we have content, show a message
  if (linesDrawn == 0 && totalResponseLines > 0) {
    display.setCursor(0, 20);
    display.print("Scroll to see more");
  }
  
  // Scroll indicator
  if (totalResponseLines > availableLines) {
    int maxScroll = totalResponseLines - availableLines;
    if (maxScroll < 0) maxScroll = 0;
    
    int scrollPos = 0;
    if (maxScroll > 0) {
      scrollPos = (scrollOffset * 40) / maxScroll;
    }
    
    display.drawRect(122, 10, 2, 40, WHITE);
    display.fillRect(122, 10 + scrollPos, 2, 8, WHITE);
    
    // Page number
    display.setCursor(110, 56);
    if (totalResponseLines > 0) {
      display.print(startLine / availableLines + 1);
      display.print("/");
      int totalPages = (totalResponseLines + availableLines - 1) / availableLines;
      display.print(totalPages);
    }
  }
  
  Serial.print("[DISPLAY] Drawn ");
  Serial.print(linesDrawn);
  Serial.print(" lines, total: ");
  Serial.print(totalResponseLines);
  Serial.print(", scroll: ");
  Serial.println(scrollOffset);
}

void drawErrorScreen(String errorMsg) {
  display.setCursor(0, 0);
  display.print("ERROR");

  display.setCursor(0, 16);
  display.print("Check serial");
  display.setCursor(0, 24);
  display.print("for details");

  if (errorMsg.length() > CHARS_PER_LINE) {
    errorMsg = errorMsg.substring(0, CHARS_PER_LINE);
  }

  display.setCursor(0, 40);
  display.print(errorMsg);

  display.setCursor(0, 56);
  display.print("Press ACTION");
}

// ========================================================================
// PROFESSIONAL BUZZER SOUNDS
// ========================================================================
void playBeep(int freq, int duration) {
  if (!buzzerEnabled || duration <= 0) return;

  int period = 1000000L / freq;
  int cycles = duration * 1000L / period;

  for (int i = 0; i < cycles; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delayMicroseconds(period / 2);
    digitalWrite(BUZZER_PIN, LOW);
    delayMicroseconds(period / 2);
  }
}

void playDotTone() {
  playBeep(1200, DOT_DURATION);
}

void playDashTone() {
  playBeep(800, DASH_DURATION);
}

void playSuccessTone() {
  playBeep(1000, 100);
  delay(30);
  playBeep(1200, 100);
  delay(30);
  playBeep(1500, 150);
}

void playErrorTone() {
  playBeep(400, 300);
  delay(100);
  playBeep(300, 400);
}

// ========================================================================
// API FUNCTIONS
// ========================================================================
String makeAPIRequest(String query) {
  Serial.println("[API] Starting API request...");

  WiFiClientSecure client;
  HTTPClient http;

  client.setInsecure();
  client.setTimeout(API_TIMEOUT);
  client.setBufferSizes(4096, 4096); // Add this line

  if (!http.begin(client, API_URL)) {
    Serial.println("[API] Failed to begin HTTP connection");
    return "ERROR: Connection failed";
  }

  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", String("Bearer ") + API_KEY);
  http.addHeader("HTTP-Referer", "https://github.com");
  http.addHeader("X-Title", "Morse AI Chatbot");

  DynamicJsonDocument requestDoc(4096);
  requestDoc["model"] = "deepseek/deepseek-chat";
  requestDoc["max_tokens"] = 600; 
  requestDoc["temperature"] = 0.5;

  JsonArray messages = requestDoc.createNestedArray("messages");

  JsonObject systemMsg = messages.createNestedObject();  
  systemMsg["role"] = "system";
  systemMsg["content"] = "You are XBOT, a calm, intelligent JARVIS-style assistant built by Jihad for an ESP8266 + OLED device. "
"Goal: give clear, practical help with a friendly tone. "
"Rules: no romance, no sexual content, no unsafe instructions, no harassment. "
"Style: short, OLED-friendly lines. Avoid repeating words. "
"Language: recognize the user's language then reply in the same language. "
"Keep replies concise: 60-140 words. Use short sentences. "
"If you are unsure, say you are unsure and suggest a safe next step. "
"Do not include markdown, emojis, or code blocks. ";

  String historyJson = getChatHistoryForAPI();
  if (historyJson.length() > 0) {
    DynamicJsonDocument historyDoc(2048);
    DeserializationError error = deserializeJson(historyDoc, "[" + historyJson + "]");

    if (!error) {
      for (JsonObject msg : historyDoc.as<JsonArray>()) {
        JsonObject historyMsg = messages.createNestedObject();
        historyMsg["role"] = msg["role"].as<String>();
        historyMsg["content"] = msg["content"].as<String>();
      }

      Serial.print("[HISTORY] Added ");
      Serial.print(historyDoc.size());
      Serial.println(" previous messages to context");
    } else {
      Serial.print("[HISTORY] Error parsing history JSON: ");
      Serial.println(error.c_str());
    }

    historyDoc.clear();
  }

  JsonObject userMsg = messages.createNestedObject();
  userMsg["role"] = "user";
  userMsg["content"] = query;

  String requestBody;
  serializeJson(requestDoc, requestBody);

  Serial.println("[API] Sending request...");
  Serial.print("[API] Request size: ");
  Serial.println(requestBody.length());

  currentState = STATE_RECEIVING;

  http.setTimeout(API_TIMEOUT);
  
  // Increase HTTP buffer size
  http.setReuse(true);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  
  int httpCode = http.POST(requestBody);

  Serial.printf("[API] HTTP Response Code: %d\n", httpCode);

  String result = "";

  if (httpCode == 200) {
    // Get response with timeout handling
    WiFiClient* stream = http.getStreamPtr();
    String payload = "";
    
    unsigned long timeout = millis();
    while (http.connected() && (millis() - timeout < 10000)) {
      while (stream->available()) {
        char c = stream->read();
        payload += c;
        timeout = millis();
      }
      delay(1);
    }
    
    Serial.print("[API] Response size: ");
    Serial.println(payload.length());
    
    // Debug: Print first 500 characters
    if (payload.length() > 500) {
      Serial.println("[API] First 500 chars of response:");
      Serial.println(payload.substring(0, 500));
    }

    requestDoc.clear();

    result = extractAndCleanResponse(payload);

    payload = "";

  } else if (httpCode == 401) {
    result = "ERROR: Invalid API key";
  } else if (httpCode == 402) {
    result = "ERROR: Add credits to account";
  } else if (httpCode == 0) {
    result = "ERROR: Connection timeout";
  } else if (httpCode == 400) {
    // Get error details
    String errorPayload = http.getString();
    Serial.print("[API] Error response: ");
    Serial.println(errorPayload);
    result = "ERROR: Bad request - check model";
  } else {
    result = "ERROR: HTTP " + String(httpCode);
  }

  http.end();
  client.stop();

  return result;
}

String extractAndCleanResponse(String payload) {
  Serial.println("[API] Starting JSON parsing...");
  Serial.print("[API] Payload length: ");
  Serial.println(payload.length());
  
  // Check if payload is empty or too short
  if (payload.length() < 10) {
    Serial.println("[API] ERROR: Payload too short!");
    return "ERROR: Empty response from API";
  }
  
  // Debug: Print first 300 and last 300 characters for debugging
  Serial.println("[API] === FIRST 300 CHARACTERS ===");
  
  // Use manual min calculation instead of min() function
  int firstPartLength = 300;
  if (payload.length() < 300) {
    firstPartLength = payload.length();
  }
  Serial.println(payload.substring(0, firstPartLength));
  
  Serial.println("[API] === LAST 300 CHARACTERS ===");
  
  // Use manual max calculation instead of max() function
  int startIdx = 0;
  if (payload.length() > 300) {
    startIdx = payload.length() - 300;
  }
  
  Serial.println(payload.substring(startIdx));

  // Increase JSON document size to handle larger responses
  DynamicJsonDocument doc(16384);  // Increased from 8192 to 16384

  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print("[API] JSON parsing error: ");
    Serial.println(error.c_str());
    Serial.print("[API] Error code: ");
    Serial.println(error.code());
    
    // Try to find the content manually as fallback
    Serial.println("[API] Trying manual extraction...");
    
    // Method 1: Look for "content" field
    int contentStart = payload.indexOf("\"content\":\"");
    if (contentStart == -1) {
      contentStart = payload.indexOf("\"content\": \"");
    }
    
    if (contentStart != -1) {
      contentStart += 11; // Length of "\"content\":\""
      int contentEnd = payload.indexOf("\"", contentStart);
      
      // Handle escaped quotes
      while (contentEnd != -1 && contentEnd > 0 && payload.charAt(contentEnd - 1) == '\\') {
        contentEnd = payload.indexOf("\"", contentEnd + 1);
      }
      
      if (contentEnd != -1 && contentEnd > contentStart) {
        String content = payload.substring(contentStart, contentEnd);
        content.replace("\\n", "\n");
        content.replace("\\\"", "\"");
        content.replace("\\\\", "\\");
        content.replace("\\t", "\t");
        content.replace("\\r", "\r");
        
        doc.clear();
        Serial.println("[API] Successfully extracted content via fallback method");
        Serial.print("[API] Extracted content length: ");
        Serial.println(content.length());
        return content;
      }
    }
    
    // Method 2: Look for "text" field (alternative)
    int textStart = payload.indexOf("\"text\":\"");
    if (textStart == -1) {
      textStart = payload.indexOf("\"text\": \"");
    }
    
    if (textStart != -1) {
      textStart += 8; // Length of "\"text\":\""
      int textEnd = payload.indexOf("\"", textStart);
      
      while (textEnd != -1 && textEnd > 0 && payload.charAt(textEnd - 1) == '\\') {
        textEnd = payload.indexOf("\"", textEnd + 1);
      }
      
      if (textEnd != -1 && textEnd > textStart) {
        String content = payload.substring(textStart, textEnd);
        content.replace("\\n", "\n");
        content.replace("\\\"", "\"");
        content.replace("\\\\", "\\");
        
        doc.clear();
        Serial.println("[API] Successfully extracted text via fallback method");
        return content;
      }
    }

    doc.clear();
    return "ERROR: Invalid JSON response. Parsing failed.";
  }

  Serial.println("[API] JSON parsed successfully");
  
  // Check if the expected structure exists
  if (!doc.containsKey("choices")) {
    Serial.println("[API] ERROR: No 'choices' key in response");
    doc.clear();
    return "ERROR: No choices in API result";
  }
  
  if (doc["choices"].size() == 0) {
    Serial.println("[API] ERROR: Empty choices array");
    doc.clear();
    return "ERROR: Empty choices array";
  }
  
  if (!doc["choices"][0].containsKey("message")) {
    Serial.println("[API] ERROR: No 'message' key in first choice");
    doc.clear();
    return "ERROR: No message in choice";
  }
  
  if (!doc["choices"][0]["message"].containsKey("content")) {
    Serial.println("[API] ERROR: No 'content' key in message");
    doc.clear();
    return "ERROR: No content in message";
  }

  // Extract the content
  String content = doc["choices"][0]["message"]["content"].as<String>();
  
  Serial.print("[API] Original content length: ");
  Serial.println(content.length());
  
  // Clear the JSON document to free memory
  doc.clear();

  // Trim whitespace
  content.trim();
  
  if (content.length() == 0) {
    Serial.println("[API] WARNING: Content is empty after trimming");
    return "ERROR: Empty content from AI";
  }

  // Clean up markdown and formatting
  content.replace("```", "");
  content.replace("**", "");
  content.replace("*", "");
  content.replace("#", "");
  content.replace("`", "");
  
  // Handle escape sequences properly
  content.replace("\\n", "\n");
  content.replace("\\\"", "\"");
  content.replace("\\\\", "\\");
  content.replace("\\t", "\t");
  content.replace("\\r", "\r");
  content.replace("\\/", "/");

  // Replace various quote characters with standard ones
  content.replace("'", "'");
  content.replace("'", "'");
  content.replace("`", "'");
  content.replace("´", "'");
  content.replace("ʼ", "'");
  content.replace("'", "'");
  
  content.replace("'", "'");
  content.replace("'", "'");
  
  content.replace("'", "'");
  content.replace("'", "'");

  // Replace various dash/hyphen characters
  content.replace("–", "-");
  content.replace("—", "-");
  content.replace("―", "-");
  content.replace("−", "-");
  content.replace("‐", "-");

  // Replace smart quotes
  content.replace("'", "'");
  content.replace("'", "'");
  content.replace("\"", "\"");
  content.replace("\"", "\"");

  // Replace accented characters with ASCII equivalents
  content.replace("á", "a");
  content.replace("Á", "A");
  content.replace("à", "a");
  content.replace("À", "A");
  content.replace("â", "a");
  content.replace("Â", "A");
  content.replace("ä", "a");
  content.replace("Ä", "A");
  content.replace("ã", "a");
  content.replace("Ã", "A");
  content.replace("å", "a");
  content.replace("Å", "A");
  content.replace("æ", "ae");
  content.replace("Æ", "AE");

  content.replace("é", "e");
  content.replace("É", "E");
  content.replace("è", "e");
  content.replace("È", "E");
  content.replace("ê", "e");
  content.replace("Ê", "E");
  content.replace("ë", "e");
  content.replace("Ë", "E");

  content.replace("í", "i");
  content.replace("Í", "I");
  content.replace("ì", "i");
  content.replace("Ì", "I");
  content.replace("î", "i");
  content.replace("Î", "I");
  content.replace("ï", "i");
  content.replace("Ï", "I");

  content.replace("ó", "o");
  content.replace("Ó", "O");
  content.replace("ò", "o");
  content.replace("Ò", "O");
  content.replace("ô", "o");
  content.replace("Ô", "O");
  content.replace("ö", "o");
  content.replace("Ö", "O");
  content.replace("õ", "o");
  content.replace("Õ", "O");
  content.replace("ø", "o");
  content.replace("Ø", "O");
  content.replace("œ", "oe");
  content.replace("Œ", "OE");

  content.replace("ú", "u");
  content.replace("Ú", "U");
  content.replace("ù", "u");
  content.replace("Ù", "U");
  content.replace("û", "u");
  content.replace("Û", "U");
  content.replace("ü", "u");
  content.replace("Ü", "U");

  content.replace("ý", "y");
  content.replace("Ý", "Y");
  content.replace("ÿ", "y");

  content.replace("ñ", "n");
  content.replace("Ñ", "N");

  content.replace("ç", "c");
  content.replace("Ç", "C");
  content.replace("č", "c");
  content.replace("Č", "C");

  content.replace("š", "s");
  content.replace("Š", "S");
  content.replace("ś", "s");
  content.replace("Ś", "S");

  content.replace("ž", "z");
  content.replace("Ž", "Z");
  content.replace("ź", "z");
  content.replace("Ź", "Z");

  content.replace("ð", "d");
  content.replace("Ð", "D");

  content.replace("þ", "th");
  content.replace("Þ", "TH");

  content.replace("ł", "l");
  content.replace("Ł", "L");

  // Currency symbols
  content.replace("€", "EUR");
  content.replace("£", "GBP");
  content.replace("¥", "JPY");
  content.replace("¢", "cents");
  content.replace("₹", "INR");
  content.replace("$", "$");

  // Mathematical symbols
  content.replace("×", "x");
  content.replace("÷", "/");
  content.replace("±", "+/-");
  content.replace("≈", "~");
  content.replace("≠", "!=");
  content.replace("≤", "<=");
  content.replace("≥", ">=");
  content.replace("∞", "infinity");

  // Punctuation and special characters
  content.replace("¡", "!");
  content.replace("¿", "?");
  content.replace("…", "...");
  content.replace("·", ".");
  content.replace("•", "*");
  content.replace("°", " deg");
  content.replace("º", " deg");
  content.replace("ª", "a");

  // Remove any remaining non-ASCII characters
  String cleanContent = "";
  for (int i = 0; i < content.length(); i++) {
    char c = content.charAt(i);
    // Allow ASCII printable characters (32-126) plus newline and tab
    if ((c >= 32 && c <= 126) || c == '\n' || c == '\t' || c == '\r') {
      cleanContent += c;
    } else if (c == '\r') {
      // Skip carriage return
      continue;
    } else {
      // Replace with space
      cleanContent += " ";
    }
  }
  content = cleanContent;

  // Clean up extra spaces
  while (content.indexOf("  ") != -1) {
    content.replace("  ", " ");
  }

  // Clean up extra newlines
  while (content.indexOf("\n\n\n") != -1) {
    content.replace("\n\n\n", "\n\n");
  }

  // Add period if missing at end
  if (content.length() > 0) {
    char lastChar = content.charAt(content.length() - 1);
    if (lastChar != '.' && lastChar != '!' && lastChar != '?' && 
        lastChar != ':' && lastChar != ';' && lastChar != '\n') {
      content += ".";
    }
  }

  // Truncate if too long for display
  if (content.length() > MAX_RESPONSE_SIZE) {
    Serial.print("[API] Response too long, truncating from ");
    Serial.print(content.length());
    Serial.print(" to ");
    Serial.println(MAX_RESPONSE_SIZE);
    
    int cutPoint = MAX_RESPONSE_SIZE;
    
    // Try to cut at a sentence boundary
    int lastPeriod = content.lastIndexOf('.', MAX_RESPONSE_SIZE);
    int lastExclaim = content.lastIndexOf('!', MAX_RESPONSE_SIZE);
    int lastQuestion = content.lastIndexOf('?', MAX_RESPONSE_SIZE);
    int lastNewline = content.lastIndexOf('\n', MAX_RESPONSE_SIZE);

    // Manual max calculation for multiple values
    int bestCut = lastPeriod;
    if (lastExclaim > bestCut) bestCut = lastExclaim;
    if (lastQuestion > bestCut) bestCut = lastQuestion;
    if (lastNewline > bestCut) bestCut = lastNewline;
    
    if (bestCut > MAX_RESPONSE_SIZE * 0.5) { // If we found a good break point
      cutPoint = bestCut + 1;
    }
    
    content = content.substring(0, cutPoint);
    content += " [Response truncated]";
  }

  Serial.print("[API] Final cleaned content length: ");
  Serial.println(content.length());
  
  return content;
}

// ========================================================================
// RESPONSE PREPARATION
// ========================================================================
void clearInput() {
  textBuffer = "";
  morseBuffer = "";
  Serial.println("\n[SYSTEM] Input cleared");
  playBeep(600, 150);
  delay(80);
  playBeep(600, 150);
}

void prepareResponseForDisplay() {
  totalResponseLines = 0;
  
  // Clear all lines
  for (int i = 0; i < 50; i++) {
    responseLines[i] = "";
  }
  
  Serial.print("[DISPLAY] Preparing response for display. Length: ");
  Serial.println(displayResponse.length());
  
  // Split response into lines
  String remaining = displayResponse;
  int lineIndex = 0;
  int charCount = 0;
  
  while (remaining.length() > 0 && lineIndex < 50) {
    // If very short, take everything
    if (remaining.length() <= CHARS_PER_LINE) {
      responseLines[lineIndex] = remaining;
      remaining = "";
      lineIndex++;
      break;
    }
    
    // Find break point (sentence end, space, or comma)
    int breakPoint = CHARS_PER_LINE;
    bool foundBreak = false;
    
    // First find sentence end (., !, ?)
    for (int i = CHARS_PER_LINE - 1; i >= CHARS_PER_LINE - 15; i--) {
      if (i < 0) break;
      if (remaining.charAt(i) == '.' || remaining.charAt(i) == '!' || remaining.charAt(i) == '?') {
        breakPoint = i + 1;
        foundBreak = true;
        break;
      }
    }
    
    // If not found, find comma or semicolon
    if (!foundBreak) {
      for (int i = CHARS_PER_LINE - 1; i >= CHARS_PER_LINE - 10; i--) {
        if (i < 0) break;
        if (remaining.charAt(i) == ',' || remaining.charAt(i) == ';' || remaining.charAt(i) == ':') {
          breakPoint = i + 1;
          foundBreak = true;
          break;
        }
      }
    }
    
    // Still not found, find space
    if (!foundBreak) {
      for (int i = CHARS_PER_LINE - 1; i >= 0; i--) {
        if (remaining.charAt(i) == ' ') {
          breakPoint = i + 1;
          foundBreak = true;
          break;
        }
      }
    }
    
    // If no break found, force cut
    if (!foundBreak) {
      breakPoint = CHARS_PER_LINE;
    }
    
    // Take line
    String line = remaining.substring(0, breakPoint);
    line.trim(); // Remove extra spaces
    
    if (line.length() > 0) {
      responseLines[lineIndex] = line;
      lineIndex++;
    }
    
    // Take remaining part
    if (breakPoint >= remaining.length()) {
      remaining = "";
    } else {
      remaining = remaining.substring(breakPoint);
      remaining.trim(); // Remove spaces at start
    }
    
    // Safety break
    if (lineIndex >= 50) {
      Serial.println("[DISPLAY] WARNING: Maximum 50 lines reached");
      break;
    }
  }
  
  totalResponseLines = lineIndex;
  
  Serial.print("[DISPLAY] Prepared ");
  Serial.print(totalResponseLines);
  Serial.println(" lines for display");
  
  // Debugging: Print first few lines
  for (int i = 0; i < min(5, totalResponseLines); i++) {
    Serial.print("Line ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(responseLines[i]);
  }
  
  // Fix scroll offset
  if (scrollOffset > totalResponseLines - 6 && totalResponseLines > 6) {
    scrollOffset = max(0, totalResponseLines - 6);
  }
  
  Serial.print("[DISPLAY] Scroll offset set to: ");
  Serial.println(scrollOffset);
}