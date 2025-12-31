const int pirPin = 13;      // Pin OUT du BS-612
 

int pirState = LOW;        // État précédent du PIR
int value = 0;             // État actuel lu

void setup() {
  Serial.begin(9600);

  pinMode(pirPin, INPUT);


  Serial.println("PIR BS-612 prêt !");
}

void loop() {
  value = digitalRead(pirPin);

  // Détection d'un front montant (passage LOW → HIGH)
  if (value == HIGH && pirState == LOW) {
    Serial.println("🚨 Mouvement détecté !");
 
    pirState = HIGH;
  }

  // Fin du mouvement (front descendant)
  else if (value == LOW && pirState == HIGH) {
    Serial.println("Mouvement terminé.");

    pirState = LOW;



  }
  delay(50);
} 