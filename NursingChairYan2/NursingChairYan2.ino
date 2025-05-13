#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "DFRobot_BloodOxygen_S.h"
#include "HX711.h"

// WiFi & MQTT config
const char* ssid = "YanLynHtet";
const char* password = "yanlinhtet1";
const char* mqtt_server = "203.150.145.68";
const int mqtt_port = 1883;
const char* mqtt_client = "SntESP32Client";
String mqtt_topic = "/681840be95326f0d8de98c3b/681840f0b173ede64c1a90e8/data";

WiFiClient espClient;
PubSubClient client(espClient);

// Sensor pins and setup
#define DOUT  19
#define SCK   18
HX711 scale;
DFRobot_BloodOxygen_S_I2C MAX30102(&Wire, 0x57);

// Flags to track sensor availability
bool oxyOK = true, scaleOK = true;

void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected");
}

void connectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect(mqtt_client)) {
      Serial.println("connected");
    } else {
      Serial.println(" failed, retrying...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Set I2C pins
  Wire.begin(21, 22);

  connectWiFi();
  client.setServer(mqtt_server, mqtt_port);

  // HX711 (load cell)
  scale.begin(DOUT, SCK);
  delay(100); // let HX711 settle
  scale.set_scale(-7050); // adjust to your calibration
  scale.tare();
  scaleOK = scale.is_ready();
  if (!scaleOK) Serial.println("❌ HX711 load cell not ready");

  // MAX30102 (I2C)
  oxyOK = MAX30102.begin();
  if (!oxyOK) Serial.println("❌ MAX30102 not detected");

  if (scaleOK || oxyOK)
    Serial.println("✅ Sensors initialized");
}

void loop() {
  if (!client.connected()) connectMQTT();
  client.loop();

  StaticJsonDocument<256> doc;

  // --- MAX30102 ---
  if (oxyOK) {
    Serial.println("🫁 Measuring SpO2/HR...");
    MAX30102.sensorStartCollect();
    float sumSpO2 = 0, sumHR = 0; int count = 0;
    unsigned long start = millis();

    while (millis() - start < 10000) {
      MAX30102.getHeartbeatSPO2();
      int spo2 = MAX30102._sHeartbeatSPO2.SPO2;
      int hr = MAX30102._sHeartbeatSPO2.Heartbeat;
      if (spo2 > 0 && hr > 0) {
        sumSpO2 += spo2;
        sumHR += hr;
        count++;
      }
      delay(1000);
    }
    MAX30102.sensorEndCollect();
    doc["avg_spo2"] = count > 0 ? sumSpO2 / count : -1;
    doc["avg_hr"] = count > 0 ? sumHR / count : -1;
    Serial.println("✅ SpO2/HR collected");

    // Ambient/internal temperature
    float temperature = MAX30102.getTemperature_C();
    doc["ambient_temp"] = temperature;
    Serial.print("🌡️ Ambient Temp: ");
    Serial.println(temperature);
  }

  // --- Load Cell ---
  if (scaleOK) {
    float weight = scale.get_units(10);  // average of 10 readings
    doc["weight"] = weight;
    Serial.print("⚖️ Weight: ");
    Serial.println(weight);
  }

  // --- MQTT Publish ---
  if (doc.size() > 0) {
    char buffer[256];
    serializeJson(doc, buffer);
    client.publish(mqtt_topic.c_str(), buffer);
    Serial.println("📤 MQTT Sent: " + String(buffer));
  }

  delay(1000);  // wait before next cycle
}