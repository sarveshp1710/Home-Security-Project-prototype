#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <PubSubClient.h>
#include <IRremote.h>
#include <LiquidCrystal.h>


#define IR_RECEIVE_PIN 4
#define LED_PIN 2

const int rs = 22, en = 23, d4 = 5, d5 = 18, d6 = 19, d7 = 21;
const char* ssid = "realme 7";
const char* password = "********";
const String correctPIN = "1234";
const int correctPinLen = 4;

const char* mqtt_server = "broker.hivemq.com";
const char* topic = "door/status";
const char* topicWarn = "door/warning";
const char* topicBreach = "door/breach";
const char* topicMode = "door/mode";

WebServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

bool doorOpen = false;
bool warningActive = false;
bool lastDoorState = false;
bool IRMode = false;
bool wifiConnected = true;

int failedAttempts = 0;
int warnCount = 0;
String IRPin = "";
int lcdPinPos=0;

unsigned long lastWiFiAttempt = 0;
unsigned long lastBlinkTime = 0;

void setup() {
  Serial.begin(115200);
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  lcd.begin(16, 2);
  pinMode(LED_PIN, OUTPUT);

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  WiFi.begin(ssid, password);
  Serial.println("Attempting WiFi connection...");

  client.setServer(mqtt_server, 1883);

  server.on("/", handleRoot);
  server.on("/unlock", HTTP_POST, handleUnlock);
  server.on("/close", handleClose);

  server.begin();
  Serial.println("Web server started");
}

void loop() {
  server.handleClient();
  client.loop();
  handleWiFiReconnect();
  lcd.setCursor(0,0);
  lcd.print(IRMode ? "IR Mode" : "Web Mode");
  if (IrReceiver.decode()) {
    if (!(IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT)) {
      int data = IrReceiver.decodedIRData.decodedRawData;

      if (data == 0xBA45FF00 || data == 0xB946FF00 || data == 0xB847FF00) {
        IRMode = !IRMode;
        Serial.println(IRMode ? "IR Mode Enabled (Offline)" : "Web Mode Enabled (Online)");
        lcd.setCursor(0,0);
        lcd.print("                ");
        lcd.setCursor(0,0);
        lcd.print(IRMode ? "IR Mode" : "Web Mode");
        lcd.setCursor(0,1);
        lcd.print("                ");
        if(!IRMode && wifiConnected){
          handleWiFiReconnect();
          lcd.setCursor(0,1);
          lcd.print(WiFi.localIP().toString());
        }
        publishDoorMode();
        lcd.setCursor(0,1);
      } else if (IRMode) {
        Serial.println(lcdPinPos);
        if(lcdPinPos==0){
          lcd.setCursor(0,1);
          lcd.print("                ");
          lcd.setCursor(0,1);
        }
        else
          lcd.setCursor(lcdPinPos,1);
        lcdPinPos++;
        handleIRInput(data);
      }
    }
    IrReceiver.resume();
  }

  handleWarningBlink();

  if (doorOpen != lastDoorState) {
    lastDoorState = doorOpen;
    publishDoorState();
  }
}

void handleWiFiReconnect() {
  if (WiFi.status() != WL_CONNECTED) {
    wifiConnected = false;
    if (millis() - lastWiFiAttempt > 5000) {
      if(!IRMode){
        Serial.println("WiFi Disconnected. Retrying...");
        lcd.setCursor(0,1);
        lcd.print("                ");
        lcd.setCursor(0,1);
        lcd.print("WiFiDisconnected");
      }
      WiFi.disconnect();
      WiFi.begin(ssid, password);
      lastWiFiAttempt = millis();
    }
  } else if (!wifiConnected) {
    wifiConnected = true;
    Serial.println("WiFi Connected! IP: " + WiFi.localIP().toString());
    if (!(IRMode)){
      lcd.setCursor(0,1);
      lcd.print("                ");
      lcd.setCursor(0,1);
      lcd.print(WiFi.localIP().toString());
    }
  }
}

void handleWarningBlink() {
  if (doorOpen && warningActive) {
    if (millis() - lastBlinkTime > 600) {
      lastBlinkTime = millis();
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      warnCount++;
      if(IRMode){
        lcd.setCursor(0,1);
        lcd.print("                ");
        lcd.setCursor(0,1);
        lcd.print("Close the Door!");
      }
      if (warnCount > 3) {
        warnCount = 0;
        publishDoorWarn();
      }
    }
  } else {
    digitalWrite(LED_PIN, doorOpen ? HIGH : LOW);
  }
}

void handleRoot() {
  serveFile("/index.html");
}

void handleUnlock() {
  if (server.hasArg("pin") && server.arg("pin") == correctPIN) {
    doorOpen = true;
    warningActive = false;
    failedAttempts = 0;
    digitalWrite(LED_PIN, HIGH);
    serveFile("/success.html");
    xTaskCreateDelayedWarning();
  } else {
    failedAttempts++;
    if (failedAttempts >= 3) {
      Serial.println("ALERT: 3 Failed Attempts!");
      publishDoorBreach();
      failedAttempts = 4;
    }
    serveFile("/failed.html");
  }
}

void handleClose() {
  doorOpen = false;
  warningActive = false;
  digitalWrite(LED_PIN, LOW);
  server.sendHeader("Location", "/");
  server.send(303);
}

void serveFile(String path) {
  File file = SPIFFS.open(path, "r");
  if (!file) {
    server.send(500, "text/plain", "File not found");
    return;
  }
  server.streamFile(file, "text/html");
  file.close();
}

void xTaskCreateDelayedWarning() {
  xTaskCreate([](void*) {
    delay(5000);
    if (doorOpen) {
      warningActive = true;
      Serial.println("Warning: Door still open.");
    }
    vTaskDelete(NULL);
  }, "DelayedWarning", 2048, NULL, 1, NULL);
}

void publishDoorState() {
  if (client.connected() || client.connect("ESP32_Client")) {
    client.publish(topic, doorOpen ? "OPEN" : "CLOSED");
    Serial.println("MQTT: Door " + String(doorOpen ? "OPEN" : "CLOSED"));
  }
}

void publishDoorWarn() {
  if (client.connected() || client.connect("ESP32_Client")) {
    client.publish(topicWarn, "Please close the door");
    Serial.println("MQTT: Warning sent");
  }
}

void publishDoorBreach() {
  if (client.connected() || client.connect("ESP32_Client")) {
    client.publish(topicBreach, "True");
    Serial.println("MQTT: Security breach alert");
  }
}

void publishDoorMode() {
  if (client.connected() || client.connect("ESP32_Client")) {
    client.publish(topicMode, IRMode ? "I R Mode" : "Web Mode");
  }
}

void handleIRInput(int data) {
  switch (data) {
    case 0xE916FF00: IRPin += "0"; Serial.print(0); lcd.print(0); break;
    case 0xF30CFF00: IRPin += "1"; Serial.print(1); lcd.print(1); break;
    case 0xE718FF00: IRPin += "2"; Serial.print(2); lcd.print(2); break;
    case 0xA15EFF00: IRPin += "3"; Serial.print(3); lcd.print(3); break;
    case 0xF708FF00: IRPin += "4"; Serial.print(4); lcd.print(4); break;
    case 0xE31CFF00: IRPin += "5"; Serial.print(5); lcd.print(5); break;
    case 0xA55AFF00: IRPin += "6"; Serial.print(6); lcd.print(6); break;
    case 0xBD42FF00: IRPin += "7"; Serial.print(7); lcd.print(7); break;
    case 0xAD52FF00: IRPin += "8"; Serial.print(8); lcd.print(8); break;
    case 0xB54AFF00: IRPin += "9"; Serial.print(9); lcd.print(9); break;
  }

  if (data == 0xEA15FF00) {
    lcdPinPos=0;
    if (correctPIN == IRPin) {
      Serial.println(" Correct PIN, door opened");
      failedAttempts = 0;
      lcd.setCursor(0,1);
      lcd.print("                ");
      lcd.setCursor(0,1);
      lcd.print("Correct PIN");
      digitalWrite(LED_PIN, HIGH);
      doorOpen = true;
      warningActive = false;
      xTaskCreateDelayedWarning();
    } else {
      Serial.println("\nWrong PIN");
      failedAttempts++;
      if (failedAttempts >= 3) {
      Serial.println("ALERT: 3 Failed Attempts!");
      publishDoorBreach();
      failedAttempts = 4;
      }
      lcd.setCursor(0,1);
      lcd.print("                ");
      lcd.setCursor(0,1);
      lcd.print("Wrong PIN");
    }
    IRPin = "";
  } else if (data == 0xF807FF00) {
    lcdPinPos=0;
    Serial.println("PIN Cleared");
    lcd.setCursor(0,1);
    lcd.print("                ");
    IRPin = "";
  } else if (data == 0xF609FF00) {
    lcdPinPos=0;
    Serial.println("Door Closed");
    lcd.print("                ");
    lcd.setCursor(0,1);
    lcd.print("Door Closed");
    digitalWrite(LED_PIN, LOW);
    doorOpen = false;
    warningActive = false;
    IRPin = "";
  }
}
