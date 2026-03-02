#define LIMIT_FRONT 22
#define LIMIT_BACK  24

void setup() {
  Serial.begin(115200);
  pinMode(LIMIT_FRONT, INPUT_PULLUP);
  pinMode(LIMIT_BACK, INPUT_PULLUP);
  Serial.println("Limit Switch Test Started...");
}

void loop() {
  bool frontHit = (digitalRead(LIMIT_FRONT) == LOW);
  bool backHit = (digitalRead(LIMIT_BACK) == LOW);

  Serial.print("Front (22): ");
  Serial.print(frontHit ? "PRESSED!  |  " : "OPEN      |  ");
  Serial.print("Back (24): ");
  Serial.println(backHit ? "PRESSED!" : "OPEN");
  
  delay(200); // Small delay to make the monitor readable
}