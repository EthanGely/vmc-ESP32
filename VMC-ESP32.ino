#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "Martin Router King";
const char* password = "I have a LAN";

const int green = 16;
const int yellow = 17;
const int red = 18;

String currentColor = "";

WebServer server(80);

void handle_On() {
  if (server.args() == 0) {
    server.send(400, "text/plain", "Missing color parameter");
    return;
  }

  String color = server.arg(0);  // Get the first argument (color)

  int pin = getPinFromColor(color);
  if (pin != -1) {
    if (pin != green) {
      digitalWrite(green, LOW);
    }
    if (pin != yellow) {
      digitalWrite(yellow, LOW);
    }
    if (pin != red) {
      digitalWrite(red, LOW);
    }
    digitalWrite(pin, HIGH);
    currentColor = color;
    server.send(200, "text/plain", color + " ON");
  } else {
    if (color == "off") {
      digitalWrite(green, LOW);
      digitalWrite(yellow, LOW);
      digitalWrite(red, LOW);
      currentColor = "ALL_OFF";
      server.send(200, "text/plain", "LIGHTS OFF");
    } else if (color == "allOn") {
      digitalWrite(green, HIGH);
      digitalWrite(yellow, HIGH);
      digitalWrite(red, HIGH);
      currentColor = "ALL_ON";
      server.send(200, "text/plain", "LIGHTS ON");
    } else if (color == "hell") {
      for (int i = 0; i < 100; i++) {
        digitalWrite(green, random(2) ? HIGH : LOW);
        digitalWrite(yellow, random(2) ? HIGH : LOW);
        digitalWrite(red, random(2) ? HIGH : LOW);
        delay(50);
      }
      currentColor = "HELL";
      server.send(200, "text/plain", "HELL");
    }
    server.send(400, "text/plain", "Invalid color");
  }
}

void handle_AllOn() {
  digitalWrite(green, HIGH);
  digitalWrite(yellow, HIGH);
  digitalWrite(red, HIGH);
  currentColor = "ALL_ON";
  server.send(200, "text/plain", "LIGHTS ON");
}

void handle_Home() {
  server.send(200, "text/plain", currentColor);
}

int getPinFromColor(String color) {
  if (color == "green") return green;
  if (color == "yellow") return yellow;
  if (color == "red") return red;
  return -1;  // Invalid color
}

void setup() {
  Serial.begin(115200);
  pinMode(green, OUTPUT);
  pinMode(yellow, OUTPUT);
  pinMode(red, OUTPUT);
  
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  
  while (WiFi.status() != WL_CONNECTED) {
    load();
    Serial.print(".");
  }
  
  digitalWrite(yellow, LOW);
  Serial.println("\nConnected to Wi-Fi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handle_Home);
  server.on("/on", handle_On);
  
  server.begin();
  startupColors();

}

void loop() {
  server.handleClient();
}

void startupColors() {
  digitalWrite(green, HIGH);
  delay(300);
  digitalWrite(green, LOW);
  digitalWrite(yellow, HIGH);
  delay(300);
  digitalWrite(yellow, LOW);
  digitalWrite(red, HIGH);
  delay(300);
  digitalWrite(red, LOW);
  delay(600);
  digitalWrite(green, HIGH);
  digitalWrite(yellow, HIGH);
  digitalWrite(red, HIGH);
  delay(200);
  digitalWrite(green, LOW);
  digitalWrite(yellow, LOW);
  digitalWrite(red, LOW);
}


void load() {
  digitalWrite(green, HIGH);
  delay(200);
  digitalWrite(yellow, HIGH);
  delay(50);
  digitalWrite(green, LOW);
  delay(150);
  digitalWrite(red, HIGH);
  delay(50);
  digitalWrite(yellow, LOW);
  delay(150);
  digitalWrite(red, LOW);
  delay(50);
}