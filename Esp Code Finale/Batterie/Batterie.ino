#include "M5TimerCAM.h"

class GestionnaireBatterie {
private:
    const float VOLT_MAX = 4.2; 
    const float VOLT_MIN = 3.3; 

public:
    float lireVoltage() {
        // Lecture en millivolts convertie en Volts
        return TimerCAM.Power.getBatteryVoltage() / 1000.0;
    }

    int lirePourcentage() {
        float v = lireVoltage();
        int pourcentage = (int)((v - VOLT_MIN) / (VOLT_MAX - VOLT_MIN) * 100);
        
        if (pourcentage > 100) pourcentage = 100;
        if (pourcentage < 0) pourcentage = 0;
        
        return pourcentage;
    }

    // La fonction que tu as demandée pour convertir en String
    String pourcentageToString() {
        return String(lirePourcentage()) + "%";
    }
};

GestionnaireBatterie batterie;

void setup() {
    Serial.begin(115200);
    
    // --- ALIMENTATION CAMÉRA ET SYSTÈME ---
    pinMode(33, OUTPUT);
    digitalWrite(33, HIGH); // Active l'alimentation générale
    
    TimerCAM.begin(true);

    Serial.println("=== DIAGNOSTIC ÉNERGIE ===");
    Serial.printf("Voltage : %.2f V\n", batterie.lireVoltage());
    Serial.println("Niveau  : " + batterie.pourcentageToString());
}

void loop() {
    // Vide
}