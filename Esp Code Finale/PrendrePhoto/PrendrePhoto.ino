#include "M5TimerCAM.h"

void setup() {
  Serial.begin(115200);

  // 1. Initialisation matérielle (indispensable)
  TimerCAM.begin();

  // 2. Initialisation spécifique du capteur
  if (!TimerCAM.Camera.begin()) {
    Serial.println("Erreur Init Camera");
    return;
  }

  // 3. Configuration des paramètres de l'image
  TimerCAM.Camera.sensor->set_pixformat(TimerCAM.Camera.sensor, PIXFORMAT_JPEG);
  TimerCAM.Camera.sensor->set_framesize(TimerCAM.Camera.sensor, FRAMESIZE_VGA);  // 640x480
  TimerCAM.Camera.sensor->set_vflip(TimerCAM.Camera.sensor, 1);                  // Rotation verticale
  TimerCAM.Camera.sensor->set_hmirror(TimerCAM.Camera.sensor, 0);                // Miroir horizontal
}
// Fonction qui capture l'image et affiche ses infos
// On passe par référence la taille et le buffer pour les utiliser après dans le MQTT
bool capturerImage(uint8_t** buffer, size_t* taille) {
  Serial.println("\n[CAMERA] Déclenchement capture...");

  if (TimerCAM.Camera.get()) {
    *taille = TimerCAM.Camera.fb->len;
    *buffer = TimerCAM.Camera.fb->buf;

    Serial.printf("[SUCCÈS] Photo capturée : %d octets\n", *taille);
    Serial.printf("[INFO] Résolution : %d x %d\n", TimerCAM.Camera.fb->width, TimerCAM.Camera.fb->height);

    return true;  // Capture réussie
  } else {
    Serial.println("[ERREUR] Échec de la capture caméra.");
    return false;  // Échec
  }
}
void loop() {
  // 4. Capture d'une image
  if (TimerCAM.Camera.get()) {

    // Accès aux données de la photo
    size_t image_size = TimerCAM.Camera.fb->len;      // Taille en octets
    uint8_t* image_buffer = TimerCAM.Camera.fb->buf;  // Pointeur vers les données binaires

    // Affichage des infos dans le moniteur série
    Serial.printf("\nCapture réussie ! Taille: %d octets\n", image_size);
    Serial.printf("Résolution: %d x %d\n", TimerCAM.Camera.fb->width, TimerCAM.Camera.fb->height);

    // --- C'est ici que tu insères ton code d'envoi (MQTT, HTTP ou SD) ---

    // 5. LIBÉRATION CRUCIAL : On libère la mémoire pour la prochaine capture
    TimerCAM.Camera.free();
    Serial.println("Mémoire caméra libérée.");
  }

  delay(5000);  // Attente 5 secondes
}