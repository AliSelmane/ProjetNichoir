#include "M5TimerCAM.h"
#include <WiFi.h>
#include <PubSubClient.h>

// Configuration WiFi
const char* ssid = "electroProjectWifi";
const char* password = "B1MesureEnv";

// Configuration MQTT
const char* mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;
const char* mqtt_client_id = "ESP32_TimerCAM_UniqueID"; // ID Unique recommandé
const char* TOPIC_IMAGE = "camera/image/data"; 
const char* TOPIC_INFO = "camera/image/info";

WiFiClient espClient;
PubSubClient client(espClient);

// Taille maximale d'un paquet
#define MQTT_CHUNK_SIZE 1024 

// --- Fonctions de Connexion ---

void setup_wifi() {
  delay(10);
  Serial.print("\nConnexion a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connecte.");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentative de connexion MQTT...");
    if (client.connect(mqtt_client_id)) {
      Serial.println("connecte!");
    } else {
      Serial.print("echec, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

// --- Configuration et Envoi ---

void setup() {
  Serial.begin(115200);
  setup_wifi();
  
  client.setServer(mqtt_server, mqtt_port);
  
  // *** CORRECTION 1 : Augmenter la taille du buffer ***
  // On met un peu plus que le chunk size pour inclure les headers MQTT
  client.setBufferSize(MQTT_CHUNK_SIZE + 100); 

  TimerCAM.begin();
  if (!TimerCAM.Camera.begin()) {
      Serial.println("Camera Init Fail");
      return;
  }
  Serial.println("Camera Init Success");

  TimerCAM.Camera.sensor->set_pixformat(TimerCAM.Camera.sensor, PIXFORMAT_JPEG);
  
  // *** CONSEIL : Commence avec une résolution plus basse pour tester ***
  // UXGA est très lourd (1600x1200). Essaye VGA (640x480) d'abord.
  TimerCAM.Camera.sensor->set_framesize(TimerCAM.Camera.sensor, FRAMESIZE_VGA); 
  
  TimerCAM.Camera.sensor->set_vflip(TimerCAM.Camera.sensor, 1);
  TimerCAM.Camera.sensor->set_hmirror(TimerCAM.Camera.sensor, 0);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); 

  // Capture image
  if (TimerCAM.Camera.get()) {
    size_t image_size = TimerCAM.Camera.fb->len;
    uint8_t *image_buffer = TimerCAM.Camera.fb->buf;
    
    Serial.printf("Image capturee. Taille: %d octets.\n", image_size);

    // 1. SIGNALER LE DEBUT
    char info_msg[50];
    sprintf(info_msg, "START:%d", image_size);
    client.publish(TOPIC_INFO, info_msg);
    
    // Petit délai pour laisser le Python se préparer
    delay(100); 

    // 2. ENVOI DES DONNEES PAR MORCEAUX
    size_t bytes_sent = 0;
    
    while (bytes_sent < image_size) {
      // Garder la connexion MQTT vivante pendant l'envoi long
      client.loop(); 

      size_t chunk_size = image_size - bytes_sent;
      if (chunk_size > MQTT_CHUNK_SIZE) {
        chunk_size = MQTT_CHUNK_SIZE;
      }
      
      // Publie le morceau
      // Note: beginPublish/write/endPublish est souvent plus stable pour le binaire
      if (client.publish(TOPIC_IMAGE, image_buffer + bytes_sent, chunk_size)) {
         // Succès du morceau
      } else {
         Serial.println("Erreur envoi morceau !");
      }
      
      bytes_sent += chunk_size;
      
      // *** CORRECTION 2 : Délai très court (ms) au lieu de 5 secondes ***
      delay(20); 
    }

    // 3. SIGNALER LA FIN
    client.publish(TOPIC_INFO, "END");
    Serial.println("Transmission terminee.");
    
    TimerCAM.Camera.free(); 
  }

  // Attendre 10 secondes avant la prochaine photo
  // Utiliser une boucle delay pour garder le MQTT actif
  for(int i=0; i<100; i++) {
    client.loop();
    delay(100);
  }
}