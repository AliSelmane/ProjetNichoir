#include "M5TimerCAM.h"
#include "esp_sleep.h"
#include <WiFi.h>
#include "driver/rtc_io.h" // Indispensable pour rtc_gpio_pullup_en

class GestionnaireEnergie {
private:
  // Changement vers la broche 13
  const gpio_num_t PIN_PIR = GPIO_NUM_13; 
  const unsigned long FACTEUR_S = 1000000ULL;

public:
  GestionnaireEnergie() {
    pinMode(PIN_PIR, INPUT_PULLUP);
  }

  void dormir(int secondes, bool reveilSurMouvement) {
    if (reveilSurMouvement) {
      Serial.println("[VEILLE] Stabilisation PIR (GPIO 13)...");
      
      // Attente que le capteur soit au repos (1)
      unsigned long debut = millis();
      while (digitalRead(PIN_PIR) == LOW && (millis() - debut < 3000)) {
        delay(50);
      }

      // Configuration du réveil sur passage à BAS (0)
      esp_sleep_enable_ext0_wakeup(PIN_PIR, 0); 
      
      // Maintien électrique du signal pendant le sommeil (Sommeil Extrême)
      if (rtc_gpio_is_valid_gpio(PIN_PIR)) {
          rtc_gpio_pullup_en(PIN_PIR);
          rtc_gpio_pulldown_dis(PIN_PIR);
      }
    }

    if (secondes > 0) {
      esp_sleep_enable_timer_wakeup(secondes * FACTEUR_S);
    }

    // --- COUPURES POUR AUTONOMIE 6 MOIS ---
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    btStop(); 

    Serial.println("[DODO] Sommeil profond actif.");
    Serial.flush();
    esp_deep_sleep_start();
  }

  void verifierSourceReveil() {
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    Serial.println("----------------------------------------------");
    Serial.print("[REVEIL] Cause : ");
    if (cause == ESP_SLEEP_WAKEUP_EXT0) Serial.println("Mouvement (GPIO 13)");
    else if (cause == ESP_SLEEP_WAKEUP_TIMER) Serial.println("Temps ecoulé");
    else Serial.println("Initialisation / Reset");
  }
};

GestionnaireEnergie energie;

void setup() {
  Serial.begin(115200);
  TimerCAM.begin(true); 

  // Stabilisation électronique du capteur au boot
  delay(2000); 

  energie.verifierSourceReveil();

  // Tes actions ici
  Serial.println("[INFO] Travail terminé.");

  // Mise en veille
  energie.dormir(0, true); 
}

void loop() {}