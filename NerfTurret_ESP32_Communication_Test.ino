void setup() {
  Serial.begin(115200);  // USB to PC
  Serial1.begin(115200); // Comm from ESP32 (Pins 18/19)
  Serial.println("Waiting for ESP32 Data...");
}

void loop() {
  // Read whatever comes in from the ESP32 and print it to the PC screen
  if (Serial1.available()) {
    char c = Serial1.read();
    Serial.print(c);
  }
}