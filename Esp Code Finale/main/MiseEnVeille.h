#include "M5TimerCAM.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_bt.h>
#include "driver/rtc_io.h"
#include "esp_system.h"


class GEnergie {
private:
  const gpio_num_t PIN_PIR = GPIO_NUM_4;
  const int PIN_LED_INFRAROUGE = 13;
  const int PIN_LED_INTERNE = 2;
  const uint64_t SEC_VER_US = 1000000ULL;

public:
  GEnergie() {

    pinMode(PIN_PIR, INPUT);

    delay(10000);
  }

  void allumerFlash(bool etat) {

    if (etat) {
      pinMode(PIN_LED_INTERNE, OUTPUT);
      digitalWrite(PIN_LED_INTERNE, HIGH);
      gpio_hold_en(GPIO_NUM_2);  // ça verouille la pine 33 a l'état haut

      pinMode(PIN_LED_INFRAROUGE, OUTPUT);
      digitalWrite(PIN_LED_INFRAROUGE, HIGH);
      gpio_hold_en(GPIO_NUM_13);  // ça verouille la pine 13 a l'état haut
    } else {
      gpio_hold_dis(GPIO_NUM_2);
      gpio_hold_dis(GPIO_NUM_13);

      pinMode(PIN_LED_INTERNE, OUTPUT);
      digitalWrite(PIN_LED_INTERNE, LOW);

      pinMode(PIN_LED_INFRAROUGE, OUTPUT);
      digitalWrite(PIN_LED_INFRAROUGE, LOW);

      gpio_hold_en(GPIO_NUM_2);
      gpio_hold_en(GPIO_NUM_13);
    }
  }

  bool surveillerPIR(int secondesMax) {
    unsigned long debut = millis();

    int pirStateLocal = digitalRead(PIN_PIR);

    Serial.printf("[PIR] Surveillance active pendant %ds...\n", secondesMax);

    while (millis() - debut < (secondesMax * 1000)) {
      int value = digitalRead(PIN_PIR);

      if (value == HIGH && pirStateLocal == LOW) {
        Serial.println("Mouvement détecté !");

        return true;
      }

      pirStateLocal = value;
    }

    Serial.println("[PIR] Aucun mouvement détecté.");
    return false;
  }
  void SleepProfond(int secondes) {

    Serial.printf("[SYSTEME] Dodo pour %d secondes...\n", secondes);

    digitalWrite(33, HIGH);
    gpio_hold_en(GPIO_NUM_33);

    esp_sleep_enable_timer_wakeup((uint64_t)secondes * SEC_VER_US);

    esp_deep_sleep_start();
  }
};