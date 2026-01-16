# \# XBOT Morse AI Chatbot (ESP8266 + OLED)

# 

# XBOT is a Morse-code–driven AI chatbot built on the ESP8266 platform.  

# It allows users to communicate with an AI model using physical buttons instead of a keyboard, making AI interaction possible on low-resource embedded hardware.

# 

# This project focuses on human–computer interaction, embedded system design, memory optimization, and practical AI integration.

# 

# ---

# 

# \## Project Motivation

# Most AI interfaces assume access to a keyboard, touchscreen, or powerful hardware.  

# XBOT challenges that assumption by answering a simple question:

# 

# \*\*Can modern AI be used through minimal physical input on constrained hardware?\*\*

# 

# Using Morse code as the input medium, XBOT demonstrates that AI interaction is possible even with:

# \- Limited memory

# \- No keyboard

# \- No touchscreen

# \- Simple button-based input

# 

# ---

# 

# \## What Makes This Project Unique

# 

# \### 1. Morse Code as a Full AI Input System

# Unlike typical Morse projects that only decode characters, XBOT uses Morse code as a \*\*complete text input interface\*\*.

# 

# \- Users type full sentences in Morse

# \- Special Morse commands trigger actions (send, backspace, clear, library)

# \- Morse input is treated as a first-class user interface, not a novelty

# 

# ---

# 

# \### 2. Embedded AI on Low-Memory Hardware

# XBOT runs on an ESP8266, which has very limited RAM.

# 

# To make AI interaction possible:

# \- Chat history is strictly limited and optimized

# \- Responses are cleaned, truncated, and reformatted

# \- Memory is actively monitored and freed during runtime

# \- Large AI responses are split into OLED-friendly lines

# 

# This makes the system stable even under heavy usage.

# 

# ---

# 

# \### 3. OLED-Optimized AI Responses

# AI responses are not displayed raw.

# 

# They are:

# \- Cleaned from markdown and formatting

# \- Converted to ASCII-safe characters

# \- Split into readable line segments

# \- Displayed with smooth scrolling

# \- Navigable using physical buttons

# 

# This ensures long AI responses remain readable on a 128×64 OLED display.

# 

# ---

# 

# \### 4. Built-in Morse Library Browser

# XBOT includes an interactive Morse library mode.

# 

# Features:

# \- Scrollable list of Morse characters

# \- One character per line for clarity

# \- Cursor-based navigation

# \- Accessible directly from Morse commands or a dedicated button

# 

# This makes XBOT both a \*\*learning tool\*\* and an \*\*AI interface\*\*.

# 

# ---

# 

# \### 5. Configurable AI Personality

# The AI behavior is controlled through a system prompt stored in the code.

# 

# \- Default personality: calm, JARVIS-style assistant

# \- Designed for short, OLED-friendly responses

# \- Users can easily modify the system prompt to customize behavior

# 

# This separation makes the system flexible without changing core logic.

# 

# ---

# 

# \## Hardware Components

# \- ESP8266 (NodeMCU)

# \- 0.96" OLED Display (SSD1306)

# \- Buttons:

# &nbsp; - Dot

# &nbsp; - Dash

# &nbsp; - Action

# &nbsp; - Scroll Up

# &nbsp; - Scroll Down

# &nbsp; - Back

# &nbsp; - Library

# \- Buzzer

# \- Breadboard and jumper wires

# 

# > Note: Pin configuration follows my personal wiring.  

# > Pins can be reassigned in the source code if needed.

# 

# ---

# 

# \## User Manual

# 

# \### Button Functions

# 

# \*\*Dot Button\*\*

# \- Inputs a Morse dot

# \- Scrolls up in menus and AI responses

# 

# \*\*Dash Button\*\*

# \- Inputs a Morse dash

# \- Scrolls down in menus and AI responses

# 

# \*\*Action Button\*\*

# \- Converts Morse to text

# \- Sends text to AI

# \- Long press clears input or chat history

# \- Exits menus

# 

# \*\*Back Button\*\*

# \- Returns from AI response view

# \- Exits library menu

# 

# \*\*Library Button\*\*

# \- Opens the Morse code reference library

# 

# ---

# 

# \### Morse Commands

# 

# | Morse Pattern | Action |

# |--------------|--------|

# | `. - . - ..` | Send message to AI |

# | `........` | Backspace |

# | `. - . -` | Insert space |

# | `----` | Clear input |

# | `-.....-` | Toggle buzzer |

# | `. - ....` | Open Morse library |

# 

# ---

# 

# \## How the System Works

# 1\. User inputs Morse code using buttons  

# 2\. Morse patterns are decoded into characters  

# 3\. Text is accumulated in an input buffer  

# 4\. On send command, the message is sent to the AI API  

# 5\. The AI response is cleaned and processed  

# 6\. Text is split into display-friendly lines  

# 7\. Response is shown on the OLED with scrolling support  

# 

# ---

# 

# \## Software Design Highlights

# \- State-machine–based system control

# \- Active memory monitoring and cleanup

# \- Optimized chat history management

# \- Manual JSON parsing fallback for reliability

# \- OLED-aware UI rendering

# \- Hardware debounce handling

# 

# ---

# 

# \## Setup Instructions

# 1\. Open `src/xbot.ino` in Arduino IDE  

# 2\. Insert:

# &nbsp;  - Your WiFi SSID

# &nbsp;  - Your WiFi password

# &nbsp;  - Your OpenRouter API key  

# 3\. Upload the code to the ESP8266  

# 4\. Power the device  

# 5\. Start typing using Morse code  

# 

# ---

# 

# \## AI Usage Disclosure

# \*\*The idea, system architecture, and integration design are my own.\*\*  

# AI tools were used strictly as an assistant for debugging, optimization, and iteration.  

# All final implementation decisions, logic, and system behavior were determined by me.

# 

# ---

# 

# \## Future Improvements

# \- ESP32 version with expanded memory

# \- Offline AI model support

# \- SD card logging

# \- Custom font rendering

# \- More efficient response compression

# 

# ---

# 

# \## Author

# HSC 2027 student from Bangladesh, learning embedded systems and AI by building real projects from ideas and curiosity.



