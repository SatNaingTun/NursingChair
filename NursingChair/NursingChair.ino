#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_MLX90614.h>
#include "DFRobot_BloodOxygen_S.h"
#include "HX711.h"

// WiFi & MQTT
const char* ssid = "YanLynHtet";
const char* password = "yanlinhtet1";
const char* mqtt_server = "203.150.145.68";
const int mqtt_port = 1883;
const char* mqtt_client = "SntESP32Client";
String mqtt_topic = "/681840be95326f0d8de98c3b/681840f0b173ede64c1a90e8/data";

WiFiClient espClient;
PubSubClient client(espClient);

// HX711 Load Cell
#define DT  4   // Change to your ESP32 GPIO
#define SCK 5   // Change to your ESP32 GPIO
HX711 scale;

// I2C Devices on GPIO 21 (SDA) and 22 (SCL)
Adafruit_MLX90614 mlx;
DFRobot_BloodOxygen_S_I2C MAX30102(&Wire, 0x57);

// Sensor flags
bool mlxOK = true, oxyOK = true, scaleOK = true;

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
      Serial.print(" failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5s...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);  // ESP32 I2C pins

  connectWiFi();
  client.setServer(mqtt_server, mqtt_port);

  // Initialize sensors
  mlxOK = mlx.begin();
  oxyOK = MAX30102.begin();

  scale.begin(DT, SCK);
  scale.set_scale(1000
  .53);  // Your calibration factor
  scale.tare();
  scaleOK = scale.is_ready();

  // Print sensor status
  if (!mlxOK) Serial.println("❌ MLX90614 not connected");
  if (!scaleOK) Serial.println("❌ Load cell not ready");
  if (!oxyOK) Serial.println("❌ MAX30102 not connected");
  if (mlxOK || scaleOK || oxyOK) Serial.println("✅ At least one sensor ready");
}

void loop() {
  if (!client.connected()) connectMQTT();
  client.loop();

  StaticJsonDocument<256> doc;

  // --- MLX90614 ---
  if (mlxOK) {
    float ambient = mlx.readAmbientTempC();
    float object = mlx.readObjectTempC();
    if (!isnan(ambient) && !isnan(object)) {
      doc["ambient_temp"] = ambient;
      doc["object_temp"] = object;
      Serial.println("🌡️ Temperature measured");
    } else {
      Serial.println("⚠️ MLX90614 read failed");
    }
  }

  // --- MAX30102 ---
  if (oxyOK) {
    Serial.println("🫁 Measuring SpO2/HR...");
    MAX30102.sensorStartCollect();
    float sumSpO2 = 0, sumHR = 0;
    int count = 0;
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
    Serial.println("✅ SpO2/HR measured");
  }

  // --- Load Cell ---
  if (scaleOK) {
    float weight = scale.get_units(10);
    doc["weight"] = weight;
    Serial.print("⚖️ Weight measured: ");
    Serial.print(weight, 2);
    Serial.println(" kg");
  }

  // --- Publish if any data ---
  if (doc.size() > 0) {
    char buffer[256];
    serializeJson(doc, buffer);
    client.publish(mqtt_topic.c_str(), buffer);
    Serial.println("📤 Sent: " + String(buffer));
  }

  delay(1000);
}