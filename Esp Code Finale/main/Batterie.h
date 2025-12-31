#include "M5TimerCAM.h"

class GestionnaireBatterie {
private:
    // Ajusté : 3.7V est maintenant considéré comme 100%
    const float VOLT_MAX = 3.7; 
    const float VOLT_MIN = 3.4; 

public:
    float lireVoltage() {
        // Lecture de la tension via l'AXP192 ou le circuit interne
        return TimerCAM.Power.getBatteryVoltage() / 1000.0;
    }

    int lirePourcentage() {
        float v = lireVoltage();
        
        // Calcul du pourcentage basé sur la nouvelle plage 3.4V - 3.7V
        int pourcentage = (int)((v - VOLT_MIN) / (VOLT_MAX - VOLT_MIN) * 100);
        
        // Bornage entre 0 et 100
        if (pourcentage > 100) pourcentage = 100;
        if (pourcentage < 0) pourcentage = 0;
        
        return pourcentage;
    }

    String pourcentageToString() {
        return String(lirePourcentage()) + "%";
    }
};