#include <Preferences.h>

void setup() {
  Serial.begin(115200);
  delay(1000);

  viderMemoireConfig();

  Serial.println("NETTOYAGE REUSSI !");
}

void loop() {
}

void viderMemoireConfig() {
  Preferences prefs;

  Serial.println("[MEMOIRE] Tentative d'ouverture de 'n-cfg'...");

  if (prefs.begin("n-cfg", false)) {

    prefs.clear();

    prefs.end();
    Serial.println("[OK] Toutes les données (SSID, Pass, Nom, Lieu) ont été effacées.");
  } else {
    Serial.println("[ERREUR] Impossible d'ouvrir le namespace 'n-cfg'.");
  }
}