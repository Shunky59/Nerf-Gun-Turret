#define HOPPER_N1 4
#define HOPPER_N2 5
#define FLY_N3    6
#define FLY_N4    7

void setup() {
  Serial.begin(115200);
  pinMode(HOPPER_N1, OUTPUT); pinMode(HOPPER_N2, OUTPUT);
  pinMode(FLY_N3, OUTPUT); pinMode(FLY_N4, OUTPUT);
  
  // Ensure N2 and N4 are tied to ground for PWM speed control on N1/N3
  digitalWrite(HOPPER_N2, LOW);
  digitalWrite(FLY_N4, LOW);
  
  Serial.println("Weapon Test: Firing sequence starting in 3 seconds...");
  delay(3000);
}

void loop() {
  Serial.println("Flywheels Spooling Up...");
  analogWrite(FLY_N3, 180); // ~70% power
  delay(3000); // Wait 3 seconds
  
  Serial.println("Hopper Feeding...");
  analogWrite(HOPPER_N1, 150); // ~60% power
  delay(1500); // Feed for 1.5 seconds
  
  Serial.println("Cease Fire. Motors OFF.");
  analogWrite(HOPPER_N1, 0);
  analogWrite(FLY_N3, 0);
  
  delay(5000); // Wait 5 seconds before repeating
}