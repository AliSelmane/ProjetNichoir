#include "M5TimerCAM.h"

class GBatterie {
private:
    const float VOLT_MAX = 3.7; 
    const float VOLT_MIN = 3.4; 

public:
    float lireVoltage() {
        return TimerCAM.Power.getBatteryVoltage() / 1000.0;
    }

    int lirePourcentage() {
        float v = lireVoltage();
        
        // Calcul du pourcentage 3.4V - 3.7V
        int pourcentage = (int)((v - VOLT_MIN) / (VOLT_MAX - VOLT_MIN) * 100);
        
        if (pourcentage > 100) pourcentage = 100;
        if (pourcentage < 0) pourcentage = 0;
        
        return pourcentage;
    }

    String pourcentageToString() {
        return String(lirePourcentage()) + "%";
    }
};