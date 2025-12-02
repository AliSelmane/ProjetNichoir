#include "esp_system.h"

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(2, OUTPUT);
}

void loop() {
  digitalWrite(2, HIGH);
}
