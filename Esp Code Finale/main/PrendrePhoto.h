#include "M5TimerCAM.h"

bool capturerImage(uint8_t** buffer, size_t* taille) {
    Serial.println("[CAMERA] Tentative de capture...");
    
    if (TimerCAM.Camera.get()) {
        *taille = TimerCAM.Camera.fb->len;
        *buffer = TimerCAM.Camera.fb->buf;
        
        Serial.printf("[SUCCÈS] Photo capturée : %d octets\n", *taille);
        return true; 
    }
    
    Serial.println("[ERREUR] Impossible de récupérer le frame buffer.");
    return false;
}