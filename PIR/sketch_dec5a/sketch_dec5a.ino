const int PIR = 13;  // À adapter selon ta vraie broche

void setup() {
  Serial.begin(9600);
  pinMode(PIR, INPUT);  // ou INPUT_PULLUP selon PIR
}

void loop() {
  int valeur = digitalRead(PIR);

  Serial.print("PIR = ");
  Serial.println(valeur);

  delay(200); // pour éviter le spam
}
