#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "Martin Router King";
const char* password = "I have a LAN";

// DTH22 SENSOR //
//const int DHT22 = 4; // power on 3.3V
const int DHT22 = 34;  // power on 3.3V

// FAN RELAYS //
const int FAN_RELAY = 23;  // Turn FAN ON / OFF

// STEPPER MOTOR //
const int STEPPER_STEP = 26;  // Moves motor
const int STEPPER_DIR = 27;   // Set motor direction

// OVERRIDE BUTTON //
const int OVERRIDE = 33;  // Override button

const int INTERRUPTED = 19;

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

void IRAM_ATTR handleButtonPress() {
  if (!isInterrupted && millis() - lastCheckTimeInt > 1500) {
    lastCheckTimeInt = millis();
    Serial.print("BUTTON PRESS : ");
    fanActiveOverride = !fanActiveOverride;
    Serial.println(fanActiveOverride);
  }
}

void handle_start() {
  if (!isInterrupted) {
    if (!fanActiveOverride) {
      if (!isFanActive) {
        fanActiveAPI = true;
        activateFan();
        server.send(200, "text/plain", "Fan Active");
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
  if (!isInterrupted) {
    if (!fanActiveOverride) {
      if (isFanActive) {
        fanActiveAPI = false;
        stopFan();
        server.send(200, "text/plain", "Fan Stopped");
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
  // DTH22 sensor
  pinMode(DHT22, INPUT);

  // FAN RELAY
  pinMode(FAN_RELAY, OUTPUT);

  // STEPPER
  pinMode(STEPPER_STEP, OUTPUT);
  pinMode(STEPPER_DIR, OUTPUT);

  // OVERRIDE BUTTON
  pinMode(OVERRIDE, INPUT_PULLUP);

  //INTEERUPTION LED
  pinMode(INTERRUPTED, OUTPUT);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
  }

  Serial.println("\nConnected to Wi-Fi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.begin();
  server.on("/start-vmc", handle_start);
  server.on("/stop-vmc", handle_stop);
  server.on("/info", handle_info);

  attachInterrupt(digitalPinToInterrupt(OVERRIDE), handleButtonPress, FALLING);


  Serial.println("----BEGIN----");
  digitalWrite(INTERRUPTED, HIGH);
  digitalWrite(FAN_RELAY, HIGH);
  digitalWrite(STEPPER_DIR, HIGH);
  digitalWrite(STEPPER_STEP, HIGH);
  delay(4000);
  digitalWrite(INTERRUPTED, LOW);
  digitalWrite(FAN_RELAY, LOW);
  digitalWrite(STEPPER_DIR, LOW);
  digitalWrite(STEPPER_STEP, LOW);
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
    int lightValue = analogRead(DHT22);
    lightValue = 4095 - lightValue;

    temperature = lightValue;
    hydrometrie = lightValue;

    
    if (fanActiveAPI) {
      if (!isFanActive) {
        Serial.println("Activating fan from API");
        activateFan();
        printInfo();
      }
      return;
    } else {
      if (lightValue < 2000) {
        if (!isFanActive) {
          Serial.println("Activating fan from SENSOR");
          activateFan();
          printInfo();
        }
        return;
      } else if (isFanActive) {
        stopFan();
        printInfo();
      }
    }
  }
}

void activateFan() {
  isInterrupted = true;
  digitalWrite(INTERRUPTED, HIGH);
  Serial.println("Opening gate...");
  digitalWrite(STEPPER_DIR, HIGH);
  digitalWrite(STEPPER_STEP, HIGH);
  delay(4000);
  Serial.println("Gate open !");
  digitalWrite(STEPPER_DIR, LOW);
  digitalWrite(STEPPER_STEP, LOW);
  digitalWrite(FAN_RELAY, HIGH);
  isFanActive = true;
  Serial.println("Fan ON");
  digitalWrite(INTERRUPTED, LOW);
  isInterrupted = false;
}

void stopFan() {
  isInterrupted = true;
  digitalWrite(INTERRUPTED, HIGH);
  Serial.println("FAN OFF");
  digitalWrite(FAN_RELAY, LOW);
  Serial.println("Waiting for fan to slow down");
  delay(2000);
  Serial.println("Closing gate...");
  digitalWrite(STEPPER_DIR, LOW);
  digitalWrite(STEPPER_STEP, HIGH);
  delay(4000);
  digitalWrite(STEPPER_STEP, LOW);
  Serial.println("Gate closed !");
  isFanActive = false;
  digitalWrite(INTERRUPTED, LOW);
  isInterrupted = false;
}

void printInfo() {
  Serial.println("---------- IMFORMATION DUMP ----------");
  Serial.print("Fan status : ");
  if (isFanActive) {
    Serial.println("1");
  } else {
    Serial.println("0");
  }
  Serial.print("Interruption : ");
  if (isInterrupted) {
    Serial.println("1");
  } else {
    Serial.println("0");
  }
  Serial.print("Override active : ");
  if (fanActiveOverride) {
    Serial.println("1");
  } else {
    Serial.println("0");
  }
  Serial.print("API active : ");
  if (fanActiveAPI) {
    Serial.println("1");
  } else {
    Serial.println("0");
  }
}
