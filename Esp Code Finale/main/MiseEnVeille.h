#include "M5TimerCAM.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include <WiFi.h>

class GestionnaireEnergie {
private:
  const gpio_num_t PIN_PIR = GPIO_NUM_4;
  const int PIN_LED_FINALE = 13;  // Flash externe (HIGH = ON)
  const int PIN_LED_TEST = 2;     // LED interne (LOW = ON sur TimerCAM)
  const uint64_t SEC_VERS_US = 1000000ULL;

public:
  GestionnaireEnergie() {

    pinMode(PIN_PIR, INPUT);  // BS-612 envoie un signal HIGH

    delay(10000);
  }

  // Gestion centralisée des LEDs (Interne + Flash)
  void allumerFlash(bool etat) {
    if (etat) {
      digitalWrite(PIN_LED_TEST, LOW);     // ON (Interne)
      digitalWrite(PIN_LED_FINALE, HIGH);  // ON (Flash)
    } else {
      digitalWrite(PIN_LED_TEST, HIGH);   // OFF
      digitalWrite(PIN_LED_FINALE, LOW);  // OFF
    }
  }

  // Surveillance PIR avec détection de front montant et délai de 2s
  bool surveillerPIR(int secondesMax) {
    unsigned long debut = millis();

    // Initialisation de l'état précédent pour détecter le front montant
    int pirStateLocal = digitalRead(PIN_PIR);

    Serial.printf("[PIR] Surveillance active pendant %ds...\n", secondesMax);

    while (millis() - debut < (secondesMax * 1000)) {
      int value = digitalRead(PIN_PIR);

      // LOGIQUE IDENTIQUE À VOTRE CODE PIR :
      // Si détection (Front montant : passe de LOW à HIGH)
      if (value == HIGH && pirStateLocal == LOW) {
        Serial.println("Mouvement détecté !");

        return true;  // On sort pour prendre la photo
      }

      pirStateLocal = value;  // Mise à jour de l'état précédent
    }

    Serial.println("[PIR] Aucun mouvement détecté.");
    return false;
  }

  void dodoProfond(int secondes) {
    Serial.printf("[SYSTEME] Dodo pour %d secondes...\n", secondes);

    digitalWrite(33, HIGH);
    gpio_hold_en(GPIO_NUM_33);

    // CORRECTION : On utilise la variable "secondes" au lieu de 20
    // On multiplie par 1 000 000 pour convertir en microsecondes
    esp_sleep_enable_timer_wakeup((uint64_t)secondes * SEC_VERS_US);

    esp_deep_sleep_start();
  }
};