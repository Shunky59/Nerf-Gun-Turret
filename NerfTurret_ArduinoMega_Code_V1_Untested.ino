// ==========================================
// NERF HYPER MACH-100 SENTRY GUN - MEGA CODE
// ==========================================

// --- Pin Definitions ---
// Pan/Tilt Motors (MDD10A)
#define PAN_DIR_PIN  8
#define PAN_PWM_PIN  9
#define TILT_DIR_PIN 2
#define TILT_PWM_PIN 11

// Weapon Motors (L298N style - applying PWM directly to N pins)
#define HOPPER_N1 4
#define HOPPER_N2 5
#define FLY_N3    6
#define FLY_N4    7

// Limit Switches (Using internal pullups)
#define LIMIT_FRONT 22 // Stops downward movement
#define LIMIT_BACK  24 // Stops upward movement

// --- Controller Data Variables ---
uint16_t joyLX = 32768, joyLY = 32768;
uint16_t joyRX = 32768, joyRY = 32768;
uint16_t trigLT = 0, trigRT = 0;
uint32_t btnMask = 0;
int dpad = 0;

// --- State Tracking & Timers ---
bool isArmed = false;
unsigned long spoolStartTime = 0;
unsigned long spindownStartTime = 0;
bool isSpinningDown = false;

const unsigned long SPOOL_TIME_MS = 7000;   // 7 seconds
const unsigned long SPINDOWN_TIME_MS = 1000; // 1 second

// --- Speeds & Tunables ---
int flywheelSpeed = 180; // Default ~70% (0-255)
int hopperSpeed = 180;   // Default ~70% (0-255)
const int JOY_DEADZONE = 4000; // Ignore stick drift

// Button Prev States (for debouncing / single-click tracking)
bool prevXboxBtn = false;
bool prevDpadUp = false;
bool prevDpadDown = false;
bool prevBtnY = false;
bool prevBtnA = false;

// --- Serial Parsing Variables ---
const byte numChars = 64;
char receivedChars[numChars];
boolean newData = false;

void setup() {
  Serial.begin(115200);   // PC Debugging
  Serial1.begin(115200);  // ESP32 Comm (RX1=19, TX1=18)

  // Motor Pins
  pinMode(PAN_DIR_PIN, OUTPUT);
  pinMode(PAN_PWM_PIN, OUTPUT);
  pinMode(TILT_DIR_PIN, OUTPUT);
  pinMode(TILT_PWM_PIN, OUTPUT);
  
  pinMode(HOPPER_N1, OUTPUT);
  pinMode(HOPPER_N2, OUTPUT);
  pinMode(FLY_N3, OUTPUT);
  pinMode(FLY_N4, OUTPUT);

  // Limit Switches (Active LOW)
  pinMode(LIMIT_FRONT, INPUT_PULLUP);
  pinMode(LIMIT_BACK, INPUT_PULLUP);

  Serial.println("Mega Sentry Gun Initialized. Awaiting ESP32...");
}

void loop() {
  recvWithStartEndMarkers();
  
  if (newData) {
    parseData();
    processControls();
    newData = false;
  }
  
  // Always update weapon motors based on timers (non-blocking)
  updateWeaponStates();
}

// ==========================================
// 1. SERIAL PARSING (NON-BLOCKING)
// ==========================================
void recvWithStartEndMarkers() {
  static boolean recvInProgress = false;
  static byte ndx = 0;
  char startMarker = '<';
  char endMarker = '>';
  char rc;

  while (Serial1.available() > 0 && newData == false) {
    rc = Serial1.read();

    if (recvInProgress == true) {
      if (rc != endMarker) {
        receivedChars[ndx] = rc;
        ndx++;
        if (ndx >= numChars) ndx = numChars - 1; // Prevent overflow
      } else {
        receivedChars[ndx] = '\0'; // Terminate string
        recvInProgress = false;
        ndx = 0;
        newData = true;
      }
    } else if (rc == startMarker) {
      recvInProgress = true;
    }
  }
}

void parseData() {
  // Format: LX,LY,RX,RY,RT,LT,Buttons,DPad
  char *strTokIndx = strtok(receivedChars, ",");
  if(strTokIndx != NULL) { joyLX = atol(strTokIndx); }
  
  strTokIndx = strtok(NULL, ",");
  if(strTokIndx != NULL) { joyLY = atol(strTokIndx); }
  
  strTokIndx = strtok(NULL, ",");
  if(strTokIndx != NULL) { joyRX = atol(strTokIndx); }
  
  strTokIndx = strtok(NULL, ",");
  if(strTokIndx != NULL) { joyRY = atol(strTokIndx); }
  
  strTokIndx = strtok(NULL, ",");
  if(strTokIndx != NULL) { trigRT = atoi(strTokIndx); }
  
  strTokIndx = strtok(NULL, ",");
  if(strTokIndx != NULL) { trigLT = atoi(strTokIndx); }
  
  strTokIndx = strtok(NULL, ",");
  if(strTokIndx != NULL) { btnMask = strtoul(strTokIndx, NULL, 10); }
  
  strTokIndx = strtok(NULL, ",");
  if(strTokIndx != NULL) { dpad = atoi(strTokIndx); }
}

// ==========================================
// 2. MAIN CONTROL LOGIC
// ==========================================
void processControls() {
  // --- AIMING LOGIC (Right Stick) ---
  float speedMultiplier = 1.0;
  
  // ADS (Aim Down Sights) via Left Trigger
  if (trigLT > 200) {
    speedMultiplier = 0.3; // Reduce speed to 30% for precise aiming
  }

  // PAN (Left / Right) -> joyRX
  int panPWM = 0;
  bool panDir = HIGH; // Assume HIGH is Right, LOW is Left (Swap if backward)
  
  if (joyRX > (32768 + JOY_DEADZONE)) {
    panDir = HIGH;
    panPWM = map(joyRX, 32768, 65535, 0, 255 * speedMultiplier);
  } else if (joyRX < (32768 - JOY_DEADZONE)) {
    panDir = LOW;
    panPWM = map(joyRX, 32768, 0, 0, 255 * speedMultiplier);
  }
  
  // TILT (Up / Down) -> joyRY
  int tiltPWM = 0;
  bool tiltDir = HIGH; // Assume HIGH is UP, LOW is DOWN (Swap if backward)
  bool hitLimit = false;
  
  // Pushing UP on stick (Usually lower raw values, adjust mapping if needed)
  if (joyRY < (32768 - JOY_DEADZONE)) {
    tiltDir = HIGH; // Moving UP
    tiltPWM = map(joyRY, 32768, 0, 0, 255 * speedMultiplier);
    
    // Check BACK limit switch
    if (digitalRead(LIMIT_BACK) == LOW) {
      tiltPWM = 0; 
      hitLimit = true;
    }
  } 
  // Pushing DOWN on stick
  else if (joyRY > (32768 + JOY_DEADZONE)) {
    tiltDir = LOW; // Moving DOWN
    tiltPWM = map(joyRY, 32768, 65535, 0, 255 * speedMultiplier);
    
    // Check FRONT limit switch
    if (digitalRead(LIMIT_FRONT) == LOW) {
      tiltPWM = 0;
      hitLimit = true;
    }
  }

  if (hitLimit) {
    Serial1.print("R"); // Tell ESP32 to rumble
  }

  // Apply to Movement Motors
  digitalWrite(PAN_DIR_PIN, panDir);
  analogWrite(PAN_PWM_PIN, panPWM);
  digitalWrite(TILT_DIR_PIN, tiltDir);
  analogWrite(TILT_PWM_PIN, tiltPWM);


  // --- WEAPON SYSTEM LOGIC ---
  
  // Button Decoding
  bool currXboxBtn = (btnMask & (1 << 6));
  bool currBtnA    = (btnMask & (1 << 0));
  bool currBtnY    = (btnMask & (1 << 3));
  bool currDpadUp  = (dpad & (1 << 0));
  bool currDpadDown= (dpad & (1 << 1));

  // 1. Safety Toggle (Xbox Button)
  if (currXboxBtn && !prevXboxBtn) {
    isArmed = !isArmed; // Toggle state
    Serial1.print("R"); // Rumble to confirm state change
    
    if (isArmed) {
      Serial.println("System ARMED. Spooling flywheels...");
      spoolStartTime = millis();
      isSpinningDown = false;
    } else {
      Serial.println("System SAFE. Spindown initiated...");
      spindownStartTime = millis();
      isSpinningDown = true;
    }
  }

  // 2. Adjust Speeds (10% Increments = ~25 PWM)
  if (currDpadUp && !prevDpadUp)     { flywheelSpeed = constrain(flywheelSpeed + 25, 0, 255); Serial.print("Flywheel: "); Serial.println(flywheelSpeed); }
  if (currDpadDown && !prevDpadDown) { flywheelSpeed = constrain(flywheelSpeed - 25, 0, 255); Serial.print("Flywheel: "); Serial.println(flywheelSpeed); }
  
  if (currBtnY && !prevBtnY) { hopperSpeed = constrain(hopperSpeed + 25, 0, 255); Serial.print("Hopper: "); Serial.println(hopperSpeed); }
  if (currBtnA && !prevBtnA) { hopperSpeed = constrain(hopperSpeed - 25, 0, 255); Serial.print("Hopper: "); Serial.println(hopperSpeed); }

  // Update previous button states for next loop
  prevXboxBtn = currXboxBtn;
  prevDpadUp = currDpadUp;
  prevDpadDown = currDpadDown;
  prevBtnY = currBtnY;
  prevBtnA = currBtnA;
}

// ==========================================
// 3. WEAPON TIMERS & MOTOR DRIVERS
// ==========================================
void updateWeaponStates() {
  if (isArmed) {
    // Flywheels run continuously at set speed
    runFlywheel(flywheelSpeed);

    // Hopper Control (Right Trigger)
    if (trigRT > 200) {
      if (millis() - spoolStartTime >= SPOOL_TIME_MS) {
        runHopper(hopperSpeed); // 7 Seconds passed, fire!
      } else {
        runHopper(0); // Too early, prevent jam
      }
    } else {
      runHopper(0); // Trigger released
    }
  } 
  else {
    // SAFE STATE
    runHopper(0); // Hopper immediately off
    
    if (isSpinningDown) {
      if (millis() - spindownStartTime < SPINDOWN_TIME_MS) {
        runFlywheel(flywheelSpeed); // Keep spinning to clear barrel
      } else {
        runFlywheel(0); // Coast to stop
        isSpinningDown = false;
      }
    } else {
      runFlywheel(0);
    }
  }
}

// Helper Functions for L298N-style operation
void runHopper(int speed) {
  analogWrite(HOPPER_N1, speed);
  digitalWrite(HOPPER_N2, LOW);
}

void runFlywheel(int speed) {
  analogWrite(FLY_N3, speed);
  digitalWrite(FLY_N4, LOW);
}