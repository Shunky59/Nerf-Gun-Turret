#include <XboxSeriesXControllerESP32_asukiaaa.hpp>

// Initialize the core object
XboxSeriesXControllerESP32_asukiaaa::Core xboxController;

unsigned long lastPrintTime = 0;

void setup() {
  Serial.begin(115200); // PC Debug
  Serial2.begin(115200, SERIAL_8N1, 16, 17); // Arduino Communication

  xboxController.begin();
  Serial.println("ESP32 Bridge Started. Ready for Controller...");
}

void loop() {
  xboxController.onLoop();

  if (xboxController.isConnected()) {
    // 1. Capture Raw Axis Values (0 to 65535)
    uint16_t lx = xboxController.xboxNotif.joyLHori;
    uint16_t ly = xboxController.xboxNotif.joyLVert;
    uint16_t rx = xboxController.xboxNotif.joyRHori;
    uint16_t ry = xboxController.xboxNotif.joyRVert;
    
    // 2. Triggers (0 to 1023)
    uint16_t lt = xboxController.xboxNotif.trigLT;
    uint16_t rt = xboxController.xboxNotif.trigRT;

    // 3. Capture Buttons into a Bitmask
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

    // 4. Capture D-Pad
    int dpad = 0;
    if (xboxController.xboxNotif.btnDirUp)    dpad |= (1 << 0);
    if (xboxController.xboxNotif.btnDirDown)  dpad |= (1 << 1);
    if (xboxController.xboxNotif.btnDirLeft)  dpad |= (1 << 2);
    if (xboxController.xboxNotif.btnDirRight) dpad |= (1 << 3);

    // 5. Send DATA Packet to Arduino (Serial2)
    // Format: <LX,LY,RX,RY,RT,LT,Buttons,DPad>
    Serial2.print("<");
    Serial2.print(lx);      Serial2.print(",");
    Serial2.print(ly);      Serial2.print(",");
    Serial2.print(rx);      Serial2.print(",");
    Serial2.print(ry);      Serial2.print(",");
    Serial2.print(rt);      Serial2.print(",");
    Serial2.print(lt);      Serial2.print(",");
    Serial2.print(btnMask); Serial2.print(",");
    Serial2.print(dpad);
    Serial2.println(">");

    // 6. Detailed Local Debug (Serial Monitor) - Every 150ms
    if (millis() - lastPrintTime > 150) {
      Serial.print("L-Joy: "); Serial.print(lx); Serial.print(","); Serial.print(ly);
      Serial.print(" | R-Joy: "); Serial.print(rx); Serial.print(","); Serial.print(ry);
      Serial.print(" | Trig: RT="); Serial.print(rt); Serial.print(" LT="); Serial.print(lt);
      Serial.print(" | DPad: "); Serial.print(dpad, BIN);
      Serial.print(" | Btns: "); Serial.println(btnMask, BIN);
      lastPrintTime = millis();
    }
  } else {
    // Failsafe to Arduino
    Serial2.println("<32768,32768,32768,32768,0,0,0,0>");
    
    if (millis() - lastPrintTime > 2000) {
      Serial.println("Searching for controller...");
      lastPrintTime = millis();
    }
  }
  delay(10); 
}