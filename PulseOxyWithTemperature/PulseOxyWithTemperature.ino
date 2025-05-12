#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include "DFRobot_BloodOxygen_S.h"
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>


// WiFi Credentials
const char* ssid = "YanLynHtet";
const char* password = "yanlinhtet1";
// const char* ssid="iot-ict-lab24g"


// MQTT Broker Info
const char* mqtt_server = "203.150.145.68";  // Your Raspberry Pi or MQTT Broker IP
const int mqtt_port = 1883;
// const char* mqtt_username = "htet";        // Optional if not using auth
// const char* mqtt_password = "raspberry";
const char* mqtt_client = "SntESP32Client";

// // Dynamic MQTT Topic Info
// const String project = "project_100";  // project_id in your topic
// const String user = "YLH100"; 
// const String device = "device_100";
// const String type = "dht_100";  
// String mqtt_topic_data = project +"/" + user+ "/" + device + "/" + type;
String mqtt_topic_data="/68071acd95326f0d8de98c37/68159d90b173ede64c1a90de/data";

// MAX30102 (Oxygen sensor)
#define I2C_ADDRESS_MAX30102 0x57
DFRobot_BloodOxygen_S_I2C MAX30102(&Wire, I2C_ADDRESS_MAX30102);

// MLX90614 (Temperature sensor)
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

// Proximity sensors
const int proxTemp = 2; // Temp trigger
const int proxOxy = 3;  // Oxygen trigger

bool tempMeasured = false;
bool oxyMeasured = false;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  pinMode(proxTemp, INPUT);
  pinMode(proxOxy, INPUT);

  // Init MLX90614
  if (!mlx.begin()) {
    Serial.println("❌ MLX90614 init failed");
    while (1);
  }
  Serial.println("✅ MLX90614 initialized");

  // Init MAX30102
  if (!MAX30102.begin()) {
    Serial.println("❌ MAX30102 init failed");
    while (1);
  }
  Serial.println("✅ MAX30102 initialized");
}

void loop() {
  // ---------- TEMPERATURE MEASUREMENT ----------
  if (digitalRead(proxTemp) == HIGH && !tempMeasured) {
    Serial.println("🕐 Waiting 1s before measuring temperature...");
    delay(1000); // Wait 1 second after proximity triggered

    float ambientTemp = mlx.readAmbientTempC();
    float objectTemp = mlx.readObjectTempC();

    Serial.println("🌡️ Temperature Measurement:");
    Serial.print("Ambient Temp: ");
    Serial.print(ambientTemp);
    Serial.println(" °C");

    Serial.print("Object Temp: ");
    Serial.print(objectTemp);
    Serial.println(" °C");

    tempMeasured = true;
    oxyMeasured = false;
    delay(1000);
  }

  // ---------- OXYGEN MEASUREMENT ----------
  if (digitalRead(proxOxy) == HIGH && !oxyMeasured && tempMeasured) {
    Serial.println("🫁 Measuring SpO2 and Heart Rate for 20 seconds...");

    MAX30102.sensorStartCollect();

    unsigned long startTime = millis();
    int count = 0;
    float sumSpO2 = 0;
    float sumHR = 0;

    while (millis() - startTime < 20000) { // 20 seconds
      MAX30102.getHeartbeatSPO2();
      int spo2 = MAX30102._sHeartbeatSPO2.SPO2;
      int hr = MAX30102._sHeartbeatSPO2.Heartbeat;

      if (spo2 > 0 && hr > 0) { // Valid readings
        sumSpO2 += spo2;
        sumHR += hr;
        count++;
      }
      delay(1000); // Read every second
    }

    MAX30102.sensorEndCollect();

    if (count > 0) {
      float avgSpO2 = sumSpO2 / count;
      float avgHR = sumHR / count;

      Serial.println("✅ Oxygen & Heart Rate Avg over 20 seconds:");
      Serial.print("Average SpO2: ");
      Serial.print(avgSpO2);
      Serial.println(" %");

      Serial.print("Average Heart Rate: ");
      Serial.print(avgHR);
      Serial.println(" bpm");
    } else {
      Serial.println("⚠️ No valid data collected during 20s.");
    }

    oxyMeasured = true;
    tempMeasured = false;
    delay(1000);
  }
}