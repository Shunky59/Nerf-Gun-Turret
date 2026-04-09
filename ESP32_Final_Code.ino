#include <XboxSeriesXControllerESP32_asukiaaa.hpp>
#include <HardwareSerial.h>

// --- UART1 PINS ---
#define RX1_PIN 16 
#define TX1_PIN 17

// Initialize the core object
XboxSeriesXControllerESP32_asukiaaa::Core xboxController;

// Timers
unsigned long lastPrintTime = 0;
unsigned long lastRecoilTime = 0;

// Local ESP32 State Tracking for Rumble
int rumbleLevel = 2; // 2 = Full (100%), 1 = Half (50%), 0 = Off (0%)
bool prevXbox = false;
bool prevView = false;  // Overlapping boxes
bool prevStart = false; // 3 lines

void setup() {
  Serial.begin(115200); // PC Debug
  
  // Start Serial1 using the defined pins for Arduino Communication
  Serial1.begin(115200, SERIAL_8N1, RX1_PIN, TX1_PIN);

  xboxController.begin();
  Serial.println("ESP32 Bridge Started. Ready for Controller...");
}

// --- RUMBLE HELPER FUNCTION ---
void triggerRumble(int durationMs, int power) {
  if (power <= 0) return; // Exit immediately if power is 0
  
  XboxSeriesXHIDReportBuilder_asukiaaa::ReportBase repo;
  
  // Turn on ALL rumble motors
  repo.v.select.left = true;   
  repo.v.select.right = true;  
  repo.v.select.center = true; 
  repo.v.select.shake = true;  
  
  // Set power (0-100)
  repo.v.power.left = power;     
  repo.v.power.right = power;    
  repo.v.power.center = power;
  repo.v.power.shake = power;
  
  // Library uses 10ms increments (e.g., 50 = 500ms)
  repo.v.timeActive = durationMs / 10; 
  if (repo.v.timeActive == 0) repo.v.timeActive = 1; // Minimum 10ms
  repo.v.timeSilent = 0;   
  repo.v.countRepeat = 1;  
  
  xboxController.writeHIDReport(repo);
}

void loop() {
  xboxController.onLoop();

  if (xboxController.isConnected()) {
    
    // ==========================================
    // 1. LOCAL ESP32 RUMBLE LOGIC
    // ==========================================
    bool currXbox = xboxController.xboxNotif.btnXbox;
    bool currView = xboxController.xboxNotif.btnSelect; // Overlapping boxes
    bool currStart = xboxController.xboxNotif.btnStart; // 3 lines
    
    // Read both triggers for ADS recoil logic
    uint16_t currRT = xboxController.xboxNotif.trigRT;
    uint16_t currLT = xboxController.xboxNotif.trigLT;

    // A. Cycle Rumble Levels (View / Overlapping Boxes)
    if (currView && !prevView) {
      rumbleLevel--; // Step down
      if (rumbleLevel < 0) rumbleLevel = 2; // Loop back to Full
      
      // Provide immediate tactile feedback for the selected level
      if (rumbleLevel == 2) {
        Serial.println("Rumble Mode: FULL");
        triggerRumble(300, 100);
      } 
      else if (rumbleLevel == 1) {
        Serial.println("Rumble Mode: HALF");
        triggerRumble(300, 50);
      } 
      else {
        Serial.println("Rumble Mode: OFF");
        // Do nothing (0% power)
      }
    }

    // Determine the baseline UI rumble power based on the current level
    int uiPower = (rumbleLevel == 2) ? 100 : ((rumbleLevel == 1) ? 50 : 0);

    // B. Safety Toggle (Xbox Button) - 500ms heavy rumble
    if (currXbox && !prevXbox) {
      triggerRumble(500, uiPower);
    }

    // C. Invert Controls Toggle (Start / 3 Lines) - 200ms quick rumble
    if (currStart && !prevStart) {
      triggerRumble(200, uiPower);
    }

    // D. Firing Recoil with ADS (Left Trigger) Logic
    if (currRT > 200) {
      if (millis() - lastRecoilTime > 100) {
        int recoilPower = 0;
        bool isADS = (currLT > 200); // Check if aiming down sights

        // Calculate recoil intensity based on current level and ADS state
        if (rumbleLevel == 2) {
          recoilPower = isADS ? 50 : 100;
        } 
        else if (rumbleLevel == 1) {
          recoilPower = isADS ? 0 : 50;
        } 
        // If rumbleLevel == 0, recoilPower remains 0

        // Fire the rumble if it's above 0
        if (recoilPower > 0) {
          triggerRumble(80, recoilPower); // Short, punchy rumble
        }
        
        lastRecoilTime = millis();
      }
    }

    // Update previous states for the next loop
    prevXbox = currXbox;
    prevView = currView;
    prevStart = currStart;


    // ==========================================
    // 2. DATA PACKET FOR ARDUINO MEGA
    // ==========================================
    uint16_t lx = xboxController.xboxNotif.joyLHori;
    uint16_t ly = xboxController.xboxNotif.joyLVert;
    uint16_t rx = xboxController.xboxNotif.joyRHori;
    uint16_t ry = xboxController.xboxNotif.joyRVert;
    
    uint32_t btnMask = 0;
    if (xboxController.xboxNotif.btnA)      btnMask |= (1 << 0);
    if (xboxController.xboxNotif.btnB)      btnMask |= (1 << 1);
    if (xboxController.xboxNotif.btnX)      btnMask |= (1 << 2);
    if (xboxController.xboxNotif.btnY)      btnMask |= (1 << 3);
    if (xboxController.xboxNotif.btnLB)     btnMask |= (1 << 4);
    if (xboxController.xboxNotif.btnRB)     btnMask |= (1 << 5);
    if (xboxController.xboxNotif.btnXbox)   btnMask |= (1 << 6);
    if (xboxController.xboxNotif.btnSelect) btnMask |= (1 << 7);
    if (xboxController.xboxNotif.btnStart)  btnMask |= (1 << 8);
    if (xboxController.xboxNotif.btnLS)     btnMask |= (1 << 9);
    if (xboxController.xboxNotif.btnRS)     btnMask |= (1 << 10);
    if (xboxController.xboxNotif.btnShare)  btnMask |= (1 << 11);

    int dpad = 0;
    if (xboxController.xboxNotif.btnDirUp)    dpad |= (1 << 0);
    if (xboxController.xboxNotif.btnDirDown)  dpad |= (1 << 1);
    if (xboxController.xboxNotif.btnDirLeft)  dpad |= (1 << 2);
    if (xboxController.xboxNotif.btnDirRight) dpad |= (1 << 3);

    // Format: <LX,LY,RX,RY,RT,LT,Buttons,DPad>
    Serial1.print("<");
    Serial1.print(lx);      Serial1.print(",");
    Serial1.print(ly);      Serial1.print(",");
    Serial1.print(rx);      Serial1.print(",");
    Serial1.print(ry);      Serial1.print(",");
    Serial1.print(currRT);  Serial1.print(",");
    Serial1.print(currLT);  Serial1.print(",");
    Serial1.print(btnMask); Serial1.print(",");
    Serial1.print(dpad);
    Serial1.println(">");

  } else {
    // Failsafe to Arduino
    Serial1.println("<32768,32768,32768,32768,0,0,0,0>");
  }
  
  delay(10); 
}