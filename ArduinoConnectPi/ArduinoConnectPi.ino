#include <SoftwareSerial.h>

SoftwareSerial mySerial(2, 3);  // RX, TX

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);
  Serial.println("Arduino Ready");
}

void loop() {
  // Send message first
  mySerial.println("Hello from Arduino");
  Serial.println("Sent: Hello from Arduino");

  delay(1000);  // Wait for response

  // Receive message
  if (mySerial.available()) {
    String received = mySerial.readString();
    Serial.print("Raspberry Pi says: ");
    Serial.println(received);
  }

  delay(1000);  // Wait before next round
}