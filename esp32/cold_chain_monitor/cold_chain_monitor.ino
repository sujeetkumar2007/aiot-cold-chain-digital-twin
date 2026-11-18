
#include "secrets.h"

#include <WiFi.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define DHTPIN 4
#define DHTTYPE DHT11

#define DS18B20_PIN 5
#define MQ135_PIN 34
#define REED_PIN 27

#define DISCOVERY_PORT 4210
#define SERVER_PORT 8000

const char* discoveryRequest = "COLDCHAIN_DISCOVER";
const char* discoveryResponse = "COLDCHAIN_SERVER";

WiFiUDP udp;

IPAddress serverIP;
bool serverFound = false;

String serverURL;

DHT dht(DHTPIN, DHTTYPE);
OneWire oneWire(DS18B20_PIN);
DallasTemperature ds18b20(&oneWire);

unsigned long lastDiscovery = 0;
const unsigned long discoveryInterval = 10000;

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

bool discoverServer() {
  Serial.println("Searching for Cold-Chain backend...");

  udp.beginPacket(IPAddress(255, 255, 255, 255), DISCOVERY_PORT);
  udp.write((const uint8_t*)discoveryRequest, strlen(discoveryRequest));
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
  Serial.println("Backend not found. Retrying later.");

  return false;
}

void setup() {
  Serial.begin(115200);

  dht.begin();
  ds18b20.begin();

  pinMode(REED_PIN, INPUT);

  connectWiFi();
  discoverServer();
}

void loop() {

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Reconnecting...");

    WiFi.disconnect();
    WiFi.begin(ssid, password);

    delay(5000);
    return;
  }

  if (!serverFound ||
      millis() - lastDiscovery >= discoveryInterval) {

    lastDiscovery = millis();
    discoverServer();

    if (!serverFound) {
      delay(2000);
      return;
    }
  }

  float humidity = dht.readHumidity();
  float dhtTemperature = dht.readTemperature();

  ds18b20.requestTemperatures();
  float temperature = ds18b20.getTempCByIndex(0);

  int gasRaw = analogRead(MQ135_PIN);
  int reedState = digitalRead(REED_PIN);

  Serial.println("\n--- COLD CHAIN SENSOR DATA ---");

  Serial.print("DHT11 Humidity: ");
  Serial.println(humidity);

  Serial.print("DHT11 Temperature: ");
  Serial.println(dhtTemperature);

  Serial.print("DS18B20 Temperature: ");
  Serial.println(temperature);

  Serial.print("MQ135 Raw: ");
  Serial.println(gasRaw);

  Serial.print("Reed State: ");
  Serial.println(reedState);

  if (isnan(humidity) || isnan(dhtTemperature) ||
      temperature == DEVICE_DISCONNECTED_C) {

    Serial.println("Sensor reading error!");
    delay(5000);
    return;
  }

  int doorOpen = (reedState == LOW) ? 1 : 0;

  float riskScore = 0;

  if (temperature > 8) riskScore += 40;
  else if (temperature > 5) riskScore += 20;

  if (humidity > 80) riskScore += 20;
  if (gasRaw > 1000) riskScore += 20;
  if (doorOpen == 1) riskScore += 20;

  if (riskScore > 100) riskScore = 100;

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
    Serial.println("Backend connection failed.");
    serverFound = false;
  }

  http.end();

  delay(5000);
}
