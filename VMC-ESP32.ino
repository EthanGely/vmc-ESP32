#include <WiFi.h>
#include <WebServer.h>
#include "DHT.h"

const char* ssid = "Martin Router King";
const char* password = "I have a LAN";

const int temperatureTreshold = 35;
const int hygrometryTreshold = 70;

// DTH22 SENSOR //
#define DHTPIN 5       // GPIO pin you're using
#define DHTTYPE DHT22   // DHT22 (AM2302)

DHT dht(DHTPIN, DHTTYPE);

// FAN RELAYS //
const int FAN_RELAY = 4;  // Turn FAN ON / OFF

// STEPPER MOTOR //
#define IN1 17
#define IN2 18
#define IN3 7
#define IN4 16

const int steps[8][4] = {
  {1, 0, 0, 0},
  {1, 1, 0, 0},
  {0, 1, 0, 0},
  {0, 1, 1, 0},
  {0, 0, 1, 0},
  {0, 0, 1, 1},
  {0, 0, 0, 1},
  {1, 0, 0, 1}
};

// OVERRIDE BUTTON //
//const int OVERRIDE = 15;  // Override button

const int INTERRUPTED = 40;

// variables
volatile bool buttonPressed = false;
bool fanActiveOverride = false;
bool fanActiveAPI = false;
bool isFanActive = false;
bool isInterrupted = false;

int temperature = 0;
int hydrometrie = 0;

unsigned long lastCheckTimeInt = 0;

WebServer server(80);

/*
void IRAM_ATTR handleButtonPress() {

    if (!isInterrupted && millis() - lastCheckTimeInt > 1500) {
    lastCheckTimeInt = millis();
    Serial.print("BUTTON PRESS : ");
    fanActiveOverride = !fanActiveOverride;
    Serial.println(fanActiveOverride);
  }
}*/


void handle_start() {
  digitalWrite(INTERRUPTED, HIGH);
  delay(500);
  digitalWrite(INTERRUPTED, LOW);
  delay(100);
  digitalWrite(INTERRUPTED, HIGH);
  delay(100);
  digitalWrite(INTERRUPTED, LOW);
  delay(100);
  digitalWrite(INTERRUPTED, HIGH);
  delay(500);
  digitalWrite(INTERRUPTED, LOW);
  if (!isInterrupted) {
    if (!fanActiveOverride) {
      if (!isFanActive) {
        fanActiveAPI = true;
        server.send(200, "text/plain", "Fan Active");
        activateFan();
        printInfo();
        return;
      } else {
        server.send(200, "text/plain", "Fan already active");
        printInfo();
        return;
      }
    } else {
        server.send(500, "text/plain", "Button is overriding");
        printInfo();
        return;
    }
  } else {
    server.send(500, "text/plain", "Already changing state");
    printInfo();
    return;
  }
  server.send(404, "text/plain", "ERROR");
  printInfo();
}

void handle_stop() {
  digitalWrite(INTERRUPTED, HIGH);
  delay(500);
  digitalWrite(INTERRUPTED, LOW);
  delay(100);
  digitalWrite(INTERRUPTED, HIGH);
  delay(100);
  digitalWrite(INTERRUPTED, LOW);
  delay(100);
  digitalWrite(INTERRUPTED, HIGH);
  delay(500);
  digitalWrite(INTERRUPTED, LOW);
  if (!isInterrupted) {
    if (!fanActiveOverride) {
      if (isFanActive) {
        fanActiveAPI = false;
        server.send(200, "text/plain", "Fan Stopped");
        stopFan(true);
        printInfo();
        return;
      } else {
        server.send(200, "text/plain", "Fan already stopped");
        printInfo();
        return;
      }
    } else {
        server.send(500, "text/plain", "Button is overriding");
        printInfo();
        return;
    }
  } else {
    server.send(500, "text/plain", "Already changing state");
    printInfo();
    return;
  }
  server.send(404, "text/plain", "ERROR");
  printInfo();
}

void handle_info() {
  printInfo();
  String message = "";

  if (isInterrupted) {
    if (isFanActive) {
      message = message + "En train de s'éteindre. ";
    } else {
      message = message + "En train de s'allummer. ";
    }
  } else {
    if (isFanActive) {
      message = message + "Vetilation ON. ";
    } else {
      message = message + "Ventialtion OFF. ";
    }
  }

  if (fanActiveOverride) {
    message = message + "Bouton override actif. ";
  }

  if (fanActiveAPI) {
    message = message + "Appel API ON. ";
  }

  server.send(200, "application/json", "{\"message\" : \"" + message + "\",\"status\" : \"" + isFanActive + "\",\"temperature\" : \"" + temperature + "\",\"hygrometrie\" : \"" + hydrometrie + "\"}");
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  // FAN RELAY
  pinMode(FAN_RELAY, OUTPUT);

  // OVERRIDE BUTTON
  //pinMode(OVERRIDE, INPUT_PULLUP); 

  //INTEERUPTION LED
  pinMode(INTERRUPTED, OUTPUT);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  stopFan(false);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    digitalWrite(INTERRUPTED, HIGH);
    delay(150);
    digitalWrite(INTERRUPTED, LOW);
  }
  digitalWrite(INTERRUPTED, LOW);
  Serial.println("\nConnected to Wi-Fi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.begin();
  server.on("/start-vmc", handle_start);
  server.on("/stop-vmc", handle_stop);
  server.on("/info", handle_info);

  //attachInterrupt(digitalPinToInterrupt(OVERRIDE), handleButtonPress, FALLING);


  Serial.println("----BEGIN----");
}

void loop() {
  static unsigned long lastCheckTime = 0;
  server.handleClient();

  if (fanActiveOverride) {
    if (!isFanActive) {
      Serial.println("Override button pressed!");
      activateFan();
      printInfo();
    }
    return;
  }

  if (millis() - lastCheckTime > 1500) {
    lastCheckTime = millis();

  hydrometrie = dht.readHumidity();
  temperature = dht.readTemperature();
    
    if (fanActiveAPI) {
      if (!isFanActive) {
        Serial.println("Activating fan from API");
        activateFan();
        printInfo();
      }
      return;
    } else {
      if (temperature >= temperatureTreshold || hydrometrie >= hygrometryTreshold) {
        if (!isFanActive) {
          Serial.println("Activating fan from SENSOR");
          activateFan();
          printInfo();
        }
        return;
      } else if (isFanActive) {
        stopFan(true);
        printInfo();
      }
    }
  }
}

void activateFan() {
  Serial.println("---START---");
  isInterrupted = true;
  digitalWrite(INTERRUPTED, HIGH);
  digitalWrite(FAN_RELAY, HIGH);
  isFanActive = true;
  turnStepper(120, true);
  digitalWrite(INTERRUPTED, LOW);
  isInterrupted = false;
}

void stopFan(bool gate) {
   Serial.println("---STOP---");
  isInterrupted = true;
  digitalWrite(INTERRUPTED, HIGH);
  digitalWrite(FAN_RELAY, LOW);

  if(gate) {
    delay(2000);
    turnStepper(120, false);
  }
  
  isFanActive = false;
  digitalWrite(INTERRUPTED, LOW);
  isInterrupted = false;
}

void printInfo() {
  //Serial.println("---------- IMFORMATION DUMP ----------");
  //Serial.print("Fan status : ");
  if (isFanActive) {
    //Serial.println("1");
  } else {
    //Serial.println("0");
  }
  //Serial.print("Interruption : ");
  if (isInterrupted) {
    //Serial.println("1");
  } else {
    //Serial.println("0");
  }
  //Serial.print("Override active : ");
  if (fanActiveOverride) {
    //Serial.println("1");
  } else {
    //Serial.println("0");
  }
  //Serial.print("API active : ");
  if (fanActiveAPI) {
    //Serial.println("1");
  } else {
    //Serial.println("0");
  }
}

void turnStepper(int deg, bool sens) {
  int degCorr = round(deg * 11.32);
  for (int i = 0; i < degCorr; i++) {  // 512 steps for a full rotation
    if (sens) {
      stepMotor(i % 8);
      delay(3);
    } else {
      int stepIndex = (8 - (i % 8)) % 8;
      stepMotor(stepIndex);
      delay(3);
    }
  }
  digitalWrite(IN1, 0);
  digitalWrite(IN2, 0);
  digitalWrite(IN3, 0);
  digitalWrite(IN4, 0);
}

void stepMotor(int step) {
  digitalWrite(IN1, steps[step][0]);
  digitalWrite(IN2, steps[step][1]);
  digitalWrite(IN3, steps[step][2]);
  digitalWrite(IN4, steps[step][3]);
}
