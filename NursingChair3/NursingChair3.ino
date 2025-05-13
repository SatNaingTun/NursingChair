#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include "HX711.h"

// Pin definitions
#define DT 19
#define SCK 18

// Sensor objects
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
HX711 scale;

void setup() {
  Serial.begin(115200);

  // Initialize I2C for MLX90614
  Wire.begin(21, 22);

  // MLX90614 setup
  if (!mlx.begin()) {
    Serial.println("❌ MLX90614 not connected");
  } else {
    Serial.println("✅ MLX90614 ready");
  }

  // HX711 setup
  scale.begin(DT, SCK);
  delay(100); // let sensor settle

  // Set calibration factor and tare
  scale.set_scale(-7050); // ⚠️ Change this value after calibration
  scale.tare();           // Set current weight as 0

  Serial.println("✅ HX711 initialized and tared");
}

void loop() {
  Serial.println("📡 Reading sensors...");

  // MLX90614 readings
  float objectTemp = mlx.readObjectTempC();
  float ambientTemp = mlx.readAmbientTempC();

  Serial.print("🌡️ Object Temp (°C): ");
  Serial.print(objectTemp, 2);
  Serial.print(" | Ambient Temp (°C): ");
  Serial.print(ambientTemp, 2);

  // HX711 readings
  if (scale.is_ready()) {
    float weight = scale.get_units(10); // Average of 10 readings
    Serial.print(" | ⚖️ Weight (kg): ");
    Serial.println(weight, 2);
  } else {
    Serial.println(" | ❌ HX711 not ready");
  }

  delay(1000);
}