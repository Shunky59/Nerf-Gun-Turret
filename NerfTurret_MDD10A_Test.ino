#define PAN_DIR_PIN  8
#define PAN_PWM_PIN  9
#define TILT_DIR_PIN 2
#define TILT_PWM_PIN 11

void setup() {
  Serial.begin(115200);
  pinMode(PAN_DIR_PIN, OUTPUT); pinMode(PAN_PWM_PIN, OUTPUT);
  pinMode(TILT_DIR_PIN, OUTPUT); pinMode(TILT_PWM_PIN, OUTPUT);
  Serial.println("Motor Test: Keep clear of moving parts!");
  delay(2000);
}

void loop() {
  // 1. Pan Left
  Serial.println("Pan Left...");
  digitalWrite(PAN_DIR_PIN, LOW); analogWrite(PAN_PWM_PIN, 100); 
  delay(1000);
  analogWrite(PAN_PWM_PIN, 0); delay(1000);

  // 2. Pan Right
  Serial.println("Pan Right...");
  digitalWrite(PAN_DIR_PIN, HIGH); analogWrite(PAN_PWM_PIN, 100); 
  delay(1000);
  analogWrite(PAN_PWM_PIN, 0); delay(1000);

  // 3. Tilt Up
  Serial.println("Tilt Up...");
  digitalWrite(TILT_DIR_PIN, HIGH); analogWrite(TILT_PWM_PIN, 100); 
  delay(1000);
  analogWrite(TILT_PWM_PIN, 0); delay(1000);

  // 4. Tilt Down
  Serial.println("Tilt Down...");
  digitalWrite(TILT_DIR_PIN, LOW); analogWrite(TILT_PWM_PIN, 100); 
  delay(1000);
  analogWrite(TILT_PWM_PIN, 0); delay(3000); // Wait before repeating
}