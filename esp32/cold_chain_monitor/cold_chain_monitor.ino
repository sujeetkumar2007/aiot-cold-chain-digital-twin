
#include "secrets.h"

#include <WiFi.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// SENSOR PINS
#define DHTPIN 4
#define DHTTYPE DHT11
#define DS18B20_PIN 5
#define MQ135_PIN 34
#define REED_PIN 27

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 display(
  SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1
);

// BACKEND DISCOVERY
#define DISCOVERY_PORT 4210

const char* discoveryRequest = "COLDCHAIN_DISCOVER";
const char* discoveryResponse = "COLDCHAIN_SERVER";

WiFiUDP udp;
IPAddress serverIP;
bool serverFound = false;
String serverURL;

// SENSORS
DHT dht(DHTPIN, DHTTYPE);
OneWire oneWire(DS18B20_PIN);
DallasTemperature ds18b20(&oneWire);

unsigned long lastDiscovery = 0;
const unsigned long discoveryInterval = 10000;

// OLED DISPLAY FUNCTION
void updateDisplay(float temperature, float humidity,
                   int gasRaw, int doorOpen, float riskScore) {

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("COLD CHAIN MONITOR");

  display.drawLine(0, 11, 127, 11, SSD1306_WHITE);

  display.setCursor(0, 15);
  display.print("Temp: ");
  display.print(temperature, 1);
  display.println(" C");

  display.setCursor(0, 25);
  display.print("Humidity: ");
  display.print(humidity, 1);
  display.println("%");

  display.setCursor(0, 35);
  display.print("Gas: ");
  display.println(gasRaw);

  display.setCursor(0, 45);
  display.print("Door: ");
  display.println(doorOpen ? "OPEN" : "CLOSED");

  display.setCursor(0, 55);
  display.print("Risk: ");
  display.print(riskScore, 0);
  display.print("% ");
  display.print(riskScore < 30 ? "LOW" :
                riskScore < 60 ? "MEDIUM" : "HIGH");

  display.display();
}

// WIFI CONNECTION
void connectWiFi() {

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected!");
  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());

  udp.begin(DISCOVERY_PORT);
}

// AUTOMATIC BACKEND DISCOVERY
bool discoverServer() {

  Serial.println("Searching for backend...");

  udp.beginPacket(
    IPAddress(255, 255, 255, 255),
    DISCOVERY_PORT
  );

  udp.write(
    (const uint8_t*)discoveryRequest,
    strlen(discoveryRequest)
  );

  udp.endPacket();

  unsigned long startTime = millis();

  while (millis() - startTime < 3000) {

    int packetSize = udp.parsePacket();

    if (packetSize > 0) {

      char incoming[64];

      int len = udp.read(incoming, sizeof(incoming) - 1);
      incoming[len] = '\0';

      if (String(incoming).startsWith(discoveryResponse)) {

        serverIP = udp.remoteIP();

        serverURL = "http://" + serverIP.toString()
                    + ":8000/api/sensor";

        serverFound = true;

        Serial.println("Backend discovered!");
        Serial.println(serverURL);

        return true;
      }
    }

    delay(10);
  }

  serverFound = false;
  Serial.println("Backend not found!");

  return false;
}

void setup() {

  Serial.begin(115200);

  // INITIALIZE OLED
  Wire.begin(21, 22);

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDRESS)) {

    Serial.println("OLED initialization failed!");
  } else {

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 20);
    display.println("COLD CHAIN STARTING");
    display.display();

    Serial.println("OLED initialized!");
  }

  // INITIALIZE SENSORS
  dht.begin();
  ds18b20.begin();

  pinMode(REED_PIN, INPUT);

  // CONNECT WIFI
  connectWiFi();

  // DISCOVER BACKEND
  discoverServer();
}

void loop() {

  if (WiFi.status() != WL_CONNECTED) {

    Serial.println("WiFi disconnected!");

    serverFound = false;
    WiFi.disconnect();
    WiFi.begin(ssid, password);

    delay(5000);
    return;
  }

  // BACKEND DISCOVERY
  if (!serverFound ||
      millis() - lastDiscovery >= discoveryInterval) {

    lastDiscovery = millis();

    if (!discoverServer()) {
      delay(2000);
      return;
    }
  }

  // READ SENSORS
  float humidity = dht.readHumidity();
  float dhtTemperature = dht.readTemperature();

  ds18b20.requestTemperatures();

  float temperature = ds18b20.getTempCByIndex(0);

  int gasRaw = analogRead(MQ135_PIN);
  int reedState = digitalRead(REED_PIN);

  if (isnan(humidity) ||
      isnan(dhtTemperature) ||
      temperature == DEVICE_DISCONNECTED_C) {

    Serial.println("Sensor reading error!");
    delay(5000);
    return;
  }

  // DOOR STATUS
  int doorOpen = (reedState == LOW) ? 1 : 0;

  // RISK SCORE (EXISTING THRESHOLD HEURISTIC)
  float riskScore = 0;

  if (temperature > 8) riskScore += 40;
  else if (temperature > 5) riskScore += 20;

  if (humidity > 80) riskScore += 20;
  if (gasRaw > 1000) riskScore += 20;
  if (doorOpen == 1) riskScore += 20;

  if (riskScore > 100) riskScore = 100;

  // UPDATE OLED
  updateDisplay(
    temperature,
    humidity,
    gasRaw,
    doorOpen,
    riskScore
  );

  // SERIAL MONITOR
  Serial.println("\n--- COLD CHAIN DATA ---");

  Serial.print("Temperature: ");
  Serial.println(temperature);

  Serial.print("Humidity: ");
  Serial.println(humidity);

  Serial.print("Gas: ");
  Serial.println(gasRaw);

  Serial.print("Door: ");
  Serial.println(doorOpen);

  Serial.print("Risk: ");
  Serial.println(riskScore);

  // SEND DATA TO BACKEND
  String jsonData = "{";

  jsonData += "\"temperature\":" + String(temperature, 2) + ",";
  jsonData += "\"humidity\":" + String(humidity, 2) + ",";
  jsonData += "\"gas_raw\":" + String(gasRaw) + ",";
  jsonData += "\"door_open\":" + String(doorOpen) + ",";
  jsonData += "\"risk_score\":" + String(riskScore, 2);

  jsonData += "}";

  HTTPClient http;

  http.setTimeout(5000);
  http.begin(serverURL);
  http.addHeader("Content-Type", "application/json");

  int responseCode = http.POST(jsonData);

  Serial.print("HTTP Response: ");
  Serial.println(responseCode);

  if (responseCode > 0) {
    Serial.println(http.getString());
  } else {
    Serial.println("Backend connection failed!");
    serverFound = false;
  }

  http.end();

  delay(5000);
}
