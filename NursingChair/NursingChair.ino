#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_MLX90614.h>
#include "MAX30105.h"          // SparkFun MAX3010x library
#include "HX711.h"

// WiFi & MQTT
const char* ssid         = "2GV31A";
const char* password     = "OTPxj8IG";
const char* mqtt_server  = "203.150.145.68";
const int   mqtt_port    = 1883;
const char* mqtt_client  = "SntESP32Client";
String      mqtt_topic   = "/681840be95326f0d8de98c3b/681840f0b173ede64c1a90e8/data";

WiFiClient espClient;
PubSubClient client(espClient);

// HX711 Load Cell
#define DT_PIN   19
#define SCK_PIN  18
HX711 scale;

// I2C Sensors on GPIO 21 (SDA) and 22 (SCL)
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
MAX30105 max30102;

// Sensor flags
bool mlxOK   = false;
bool oxyOK   = false;
bool scaleOK = false;

void connectWiFi() {
  Serial.print("Connecting to WiFi ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }
  Serial.println("\n✅ WiFi connected, IP = " + WiFi.localIP().toString());
}

void connectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT broker...");
    if (client.connect(mqtt_client)) {
      Serial.println("✅ connected");
    } else {
      Serial.print("❌ failed, rc=");
      Serial.print(client.state());
      Serial.println(" — retrying in 5 s");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // For Serial Monitor readiness

  // Start I2C once
  Wire.begin(21, 22);
  delay(500);  // Let sensors power up

  connectWiFi();
  client.setServer(mqtt_server, mqtt_port);

  // Initialize HX711
  scale.begin(DT_PIN, SCK_PIN);
  scale.set_scale(1000.53);  // Adjust based on calibration
  scale.tare();
  scaleOK = scale.is_ready();
  Serial.print("HX711 init: ");
  Serial.println(scaleOK ? "✅ OK" : "❌ FAILED");

  // Initialize MLX90614
  mlxOK = mlx.begin();
  Serial.print("MLX90614 init: ");
  Serial.println(mlxOK ? "✅ OK" : "❌ FAILED");

  // Initialize MAX30102
  oxyOK = max30102.begin(Wire, I2C_SPEED_STANDARD);
  Serial.print("MAX30102 init: ");
  Serial.println(oxyOK ? "✅ OK" : "❌ FAILED");

  if (oxyOK) {
    max30102.setup();  // Default config: HR and SpO2
  }
}

void loop() {
  if (!client.connected()) {
    connectMQTT();
  }
  client.loop();

  StaticJsonDocument<256> doc;

  // MLX90614
  if (mlxOK) {
    float ambient = mlx.readAmbientTempC();
    float object = mlx.readObjectTempC();
    if (!isnan(ambient) && !isnan(object)) {
      doc["ambient_temp"] = ambient;
      doc["object_temp"]  = object;
      Serial.println("🌡️ MLX90614 OK");
    } else {
      Serial.println("⚠️ MLX90614 read failed");
    }
  }

  // MAX30102 (simplified)
  if (oxyOK) {
    float sumSpO2 = 0, sumHR = 0;
    int count = 0;
    unsigned long start = millis();
    while (millis() - start < 10000) {
      long ir = max30102.getIR();
      if (ir < 50000) {
        Serial.println("⚠️ No finger detected");
      } else {
        sumSpO2 += 97;  // Dummy constant, replace with real algorithm if needed
        sumHR   += 72;
        count++;
      }
      delay(1000);
    }
    doc["avg_spo2"] = (count > 0 ? sumSpO2 / count : -1);
    doc["avg_hr"]   = (count > 0 ? sumHR / count   : -1);
    Serial.println("✅ MAX30102 OK");
  }

  // HX711
  if (scaleOK) {
    float weight = scale.get_units(10);
    doc["weight"] = weight;
    Serial.print("⚖️ Weight: ");
    Serial.print(weight, 2);
    Serial.println(" kg");
  }

  // Publish
  if (doc.size() > 0) {
    char buffer[256];
    serializeJson(doc, buffer);
    client.publish(mqtt_topic.c_str(), buffer);
    Serial.println("📤 Published JSON: " + String(buffer));
  }

  delay(1000);
}
