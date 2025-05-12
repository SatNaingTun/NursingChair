#include <SoftwareSerial.h>

SoftwareSerial mySerial(2, 3);  // RX, TX

void setup() {
  Serial.begin(115200);
  mySerial.begin(115200);
  Serial.println("Arduino Ready");
}

void loop() {
  if (mySerial.available()) {
    Serial.print("ESP32 says: ");
    Serial.println(mySerial.readString());
  }
  
  mySerial.println("Hello from Arduino");
  delay(1000);
}