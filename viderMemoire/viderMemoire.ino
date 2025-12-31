#include <Preferences.h>

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  viderMemoireConfig();
  
  Serial.println("\n************************************************");
  Serial.println("NETTOYAGE REUSSI !");
  Serial.println("Le namespace 'n-cfg' a été vidé.");
  Serial.println("Tu peux maintenant téléverser ton code principal.");
  Serial.println("************************************************");
}

void loop() {
  // Boucle vide pour éviter de répéter l'opération
}

void viderMemoireConfig() {
  Preferences prefs;
  
  Serial.println("[MEMOIRE] Tentative d'ouverture de 'n-cfg'...");

  // Ouverture du namespace que tu utilises : "n-cfg"
  if (prefs.begin("n-cfg", false)) {
    
    // On efface tout ce qui est dans "n-cfg" (s, p, n, l)
    prefs.clear(); 
    
    prefs.end();
    Serial.println("[OK] Toutes les données (SSID, Pass, Nom, Lieu) ont été effacées.");
  } else {
    Serial.println("[ERREUR] Impossible d'ouvrir le namespace 'n-cfg'.");
  }
}