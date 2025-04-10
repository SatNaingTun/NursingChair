#include <Wire.h>
#include <Adafruit_MLX90614.h>

// Create sensor object
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

void setup() {
  Serial.begin(9600);
  Serial.println("MLX90614 IR Temperature Sensor with Arduino Mega");

  if (!mlx.begin()) {
    Serial.println("Error connecting to MLX90614. Check wiring!");
    while (1); // stay here if sensor isn't found
  }
}

void loop() {
  Serial.print("Ambient Temp: ");
  Serial.print(mlx.readAmbientTempC());
  Serial.print(" °C\t");

  Serial.print("Object Temp: ");
  Serial.print(mlx.readObjectTempC());
  Serial.println(" °C");

  delay(1000);
}

