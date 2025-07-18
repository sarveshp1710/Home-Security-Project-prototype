# 🔐 ESP32 Smart Door Lock with Dual Control (IR & Web) + MQTT Logging

This project is a smart door locking system using an ESP32 that allows you to:

- ✅ Unlock the door with a 4-digit PIN using either:
  - A website (via phone/laptop over Wi-Fi)
  - An IR remote (offline access)
- 🌐 Send door open/close status over MQTT for logging and monitoring
- 🔁 Recover automatically from Wi-Fi disconnections without blocking the IR interface

---

## 🚀 Features

- 🧠 Dual-mode control: IR remote and web interface
- 📢 Voice feedback using node-red dashboard
  - PIN success → "Door Opened"
  - PIN failure → "Security breach detected" (after 3 incorrect attempts)
  - Close Door → "Door Closed"
  - Left opened for 5 seconds → "Please Close the door"
- 📶 MQTT status updates for door state (`OPEN`, `CLOSED`)
- ✅ Prevents using browser back button to avoid retry abuse
- 🔄 Non-blocking design for smooth IR + Wi-Fi multitasking
- 🔒 Anti-brute-force: Alert email is sent to the verified owner after 3 incorrect attempts
- 🔧 Easy to extend for real door hardware (currently uses onboard LED to simulate lock)
- ✅ Warns the user if the door is left open for more than 5 seconds (configurable)
- ✅ Visual feedback with LEDs and animated web pages
- ✅ Activity log with timestamps (can be extended to Google Sheets)

---

## 🌐 Web Interface

- Hosted directly from ESP32 SPIFFS
- Accessible via IP address on the same Wi-Fi network or port-forwarded for internet access
- Clean UI for PIN input and door status display
- Success and failure pages with countdown and redirect
- Prevents browser back misuse

---

## 📡 IR Remote Control

- Non-blocking IR decoding using `IRremote` library
- PIN entry using remote number keys
- Clear (`*`) and OK (`#`) functionality
- Mode can be switched using special remote buttons.
- Open and Close door action also can be done using special remote buttons.

---

## 🛠 Technologies Used

- ESP32 (30 pins)
- HTML, CSS, JavaScript (hosted via ESP32 web server)
- IR Remote Library
- Node-RED (for MQTT logging, voice feedback)
- MQTT
- SPIFFS (Serial Peripheral Interface Flash File System)

---

# 🗂️ File System (SPIFFS)
All HTML files for the web interface are uploaded to SPIFFS using the Arduino IDE SPIFFS Uploader. This allows the ESP32 to serve a full-featured responsive web app accessible from any browser and simplifies the future development process.

---

## 🚀 How It Works

1. Enter the PIN through the web page or IR remote.(web mode by default)
2. If the PIN is correct:
   - Door unlocks.
   - LED indicator glows.
   - Success page shows with countdown.
   - Status is logged and spoken via Node-RED.
3. If incorrect:
   - Access Denied page shows and counts the attempts.
   - if incorrect attempts exceeds 3, it sends alert email to the authorized owner.(NO 3rd party online platforms like blynk are used)
4. Door can be closed in Either of the modes.
5. MQTT logs and voice alerts are triggered in Node-RED.
6. Warns the user if the door is left open for more than 5 seconds (configurable).

---

## 🌐 Accessing Over the Internet

To control the system from outside your local network:

- Use MQTT with a cloud broker (eg. HiveMQ).
- Or enable port forwarding for the ESP32 web server.(eg. ngrok)
- Ensure secure authentication and avoid exposing sensitive ports.

---

## 📁 Files & Folders

- /web/index.html – Main keypad interface
- /web/success.html – Access granted response page
- /web/fail.html – Access denied response page
- /src/main.ino – ESP32 source code
- /node-red/flows.json – Node-RED flow for MQTT + TTS
- /README.md – You're here!

---

# 🛠 Hardware Connections

E-ESP32(30 PINS), L-LCD Display 16x2 (Without I2C), I-IR Receiver Module, P-Potentiometer <br>
<br>
I-VCC --> E-3.3v<br>
I-GND --> E-GND<br>
I-OUT --> E-D4<br>
<br>
E-D5,D18,D-19,D21 --> L-D4,D5,D6,D7<br>
L-VSS, L-LED-GND, L-RW --> E-GND<br>
L-VCC, L-LED 5V+ --> E-VIN(5V)<br>
L-RS --> E-D22<br>
L-E --> E-D23 <br>
<br>
L-VEE --> P-MIDDLE<br>
E-VIN(5V) --> P-LEFT<br>
E-GND --> P-RIGHT<br>

---

# Demo Video Link
Linkedin: https://www.linkedin.com/posts/sarvesh-p-981575288_iotprojects-smartsecurity-esp32-activity-7350212581845843968-HJiC?utm_source=share&utm_medium=member_desktop&rcm=ACoAAEXZ2jEB_DadRBjJA1c9TofHQMtb9QV1XBU

---

## 🙌 Credits

Developed by: <b>Sarvesh P </b><br>
Voice feedback: Node-RED<br>
Communication: MQTT<br>
ESP32 Web Control & IR Mode: Custom embedded firmware <br>

<h1><centre>Stay Creative!</centre></h1>
