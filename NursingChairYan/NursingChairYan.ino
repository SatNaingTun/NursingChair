#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include "HX711.h"

#define DT 19
#define SCK 18

Adafruit_MLX90614 mlx = Adafruit_MLX90614();
HX711 scale;

void setup() {
  Serial.begin(115200);
  
  // Initialize I2C
  Wire.begin(21, 22); // ESP32 default: SDA=21, SCL=22

  // Initialize MLX sensor
  if (!mlx.begin()) {
    Serial.println("❌ MLX90614 not connected");
  } else {
    Serial.println("✅ MLX90614 ready");
  }

  // Initialize HX711
  scale.begin(DT, SCK);
  Serial.println("✅ HX711 initialized");
}

void loop() {
  Serial.print("Temp (°C): ");
  Serial.print(mlx.readObjectTempC());

  if (scale.is_ready()) {
    Serial.print(" | Weight: ");
    Serial.print(scale.get_units(), 1); // Adjust calibration factor as needed
  } else {
    Serial.print(" | HX711 not ready");
  }

  Serial.println();
  delay(1000);
}
