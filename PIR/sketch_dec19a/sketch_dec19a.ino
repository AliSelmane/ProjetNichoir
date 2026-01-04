const int pirPin = 4;      
 

int pirState = LOW;       
int value = 0;             

void setup() {
  Serial.begin(9600);

  pinMode(pirPin, INPUT);


  Serial.println("PIR BS-612 prêt !");
}

void loop() {
  value = digitalRead(pirPin);

  if (value == HIGH && pirState == LOW) {
    Serial.println("Mouvement détecté !");
 
    pirState = HIGH;
  }

  else if (value == LOW && pirState == HIGH) {
    Serial.println("Mouvement terminé.");

    pirState = LOW;



  }
  delay(50);
} 