#include "M5TimerCAM.h"

void setup() {
  TimerCAM.begin(true);
}

void loop() {
  Serial.printf("Bat Voltage: %dmv\r\n", TimerCAM.Power.getBatteryVoltage());
  Serial.printf("Bat Level: %d%%\r\n", TimerCAM.Power.getBatteryLevel());
  delay(1000);
}