#include "secrets.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define DHTPIN 4
#define DHTTYPE DHT11

#define DS18B20_PIN 5
#define MQ135_PIN 34
#define REED_PIN 27


const char* serverURL = "http://192.168.252.97:8000/api/sensor";

DHT dht(DHTPIN, DHTTYPE);

OneWire oneWire(DS18B20_PIN);
DallasTemperature ds18b20(&oneWire);

void setup() {
  Serial.begin(115200);

  dht.begin();
  ds18b20.begin();

  pinMode(REED_PIN, INPUT);

  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Connected!");
  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {

  float humidity = dht.readHumidity();
  float dhtTemperature = dht.readTemperature();

  ds18b20.requestTemperatures();
  float temperature = ds18b20.getTempCByIndex(0);

  int gasRaw = analogRead(MQ135_PIN);
  int reedState = digitalRead(REED_PIN);

  if (isnan(humidity) || isnan(dhtTemperature) ||
      temperature == DEVICE_DISCONNECTED_C) {
    Serial.println("Sensor reading error!");
    delay(5000);
    return;
  }

  // Initial door-state assumption; verify using Serial Monitor
  int doorOpen = (reedState == LOW) ? 1 : 0;

  // Initial demonstration risk score, not a calibrated food-safety model
  float riskScore = 0;

  if (temperature > 8) riskScore += 40;
  else if (temperature > 5) riskScore += 20;

  if (humidity > 80) riskScore += 20;

  if (gasRaw > 1000) riskScore += 20;

  if (doorOpen == 1) riskScore += 20;

  if (riskScore > 100) riskScore = 100;

  Serial.println("\n--- COLD CHAIN SENSOR DATA ---");

  Serial.print("DS18B20 Temperature: ");
  Serial.println(temperature);

  Serial.print("DHT11 Temperature: ");
  Serial.println(dhtTemperature);

  Serial.print("Humidity: ");
  Serial.println(humidity);

  Serial.print("MQ135 Raw: ");
  Serial.println(gasRaw);

  Serial.print("Reed Raw State: ");
  Serial.println(reedState);

  Serial.print("Door Open: ");
  Serial.println(doorOpen);

  Serial.print("Risk Score: ");
  Serial.println(riskScore);

  if (WiFi.status() == WL_CONNECTED) {

    HTTPClient http;

    http.begin(serverURL);
    http.addHeader("Content-Type", "application/json");

    String jsonData = "{";
    jsonData += "\"temperature\":" + String(temperature, 2) + ",";
    jsonData += "\"humidity\":" + String(humidity, 2) + ",";
    jsonData += "\"gas_raw\":" + String(gasRaw) + ",";
    jsonData += "\"door_open\":" + String(doorOpen) + ",";
    jsonData += "\"risk_score\":" + String(riskScore, 2);
    jsonData += "}";

    int httpResponseCode = http.POST(jsonData);

    Serial.print("HTTP Response: ");
    Serial.println(httpResponseCode);

    if (httpResponseCode > 0) {
      Serial.println(http.getString());
    }

    http.end();

  } else {
    Serial.println("WiFi disconnected!");
  }

  delay(5000);
}
