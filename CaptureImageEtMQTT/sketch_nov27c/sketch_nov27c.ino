#include "M5TimerCAM.h"
#include <WiFi.h>
#include <PubSubClient.h>

// ----------------------------------------------------
// CONFIGURATION UTILISATEUR
// ----------------------------------------------------
const char* ssid = "electroProjectWifi";
const char* password = "B1MesureEnv";

const char* mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;

// ID unique pour la connexion MQTT (évite les conflits)
const char* mqtt_client_id = "ESP32_TimerCAM_Sender";

// Topics
const char* TOPIC_IMAGE = "camera/image/data"; 
const char* TOPIC_INFO = "camera/image/info";

// ----------------------------------------------------
// VARIABLES GLOBALES
// ----------------------------------------------------
WiFiClient espClient;
PubSubClient client(espClient);

// Taille du paquet de données (Chunk). 
// IMPORTANT: Doit être plus petit que le Buffer Size défini dans setup()
#define MQTT_CHUNK_SIZE 1024 

// ----------------------------------------------------
// CONNEXION WIFI
// ----------------------------------------------------
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
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
}

// ----------------------------------------------------
// RECONNEXION MQTT
// ----------------------------------------------------
void reconnect() {
  while (!client.connected()) {
    Serial.print("Connexion MQTT...");
    if (client.connect(mqtt_client_id)) {
      Serial.println("Reussi!");
    } else {
      Serial.print("Echec, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

// ----------------------------------------------------
// SETUP
// ----------------------------------------------------
void setup() {
  Serial.begin(115200);
  setup_wifi();
  
  client.setServer(mqtt_server, mqtt_port);
  
  // *** CRUCIAL *** : On augmente la taille du buffer d'envoi MQTT
  // 1024 (données) + 200 (marge pour les entêtes du protocole)
  client.setBufferSize(MQTT_CHUNK_SIZE + 200); 

  // Init Caméra M5
  TimerCAM.begin();
  if (!TimerCAM.Camera.begin()) {
      Serial.println("Erreur Init Camera");
      return;
  }
  Serial.println("Camera Init OK");

  // Configuration Image
  TimerCAM.Camera.sensor->set_pixformat(TimerCAM.Camera.sensor, PIXFORMAT_JPEG);
  // Utilise FRAMESIZE_VGA (640x480) pour commencer (plus stable en MQTT public)
  TimerCAM.Camera.sensor->set_framesize(TimerCAM.Camera.sensor, FRAMESIZE_VGA); 
  TimerCAM.Camera.sensor->set_vflip(TimerCAM.Camera.sensor, 1);
  TimerCAM.Camera.sensor->set_hmirror(TimerCAM.Camera.sensor, 0);
}

// ----------------------------------------------------
// LOOP
// ----------------------------------------------------
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); 

  // Capture d'une image
  if (TimerCAM.Camera.get()) {
    size_t image_size = TimerCAM.Camera.fb->len;
    uint8_t *image_buffer = TimerCAM.Camera.fb->buf;
    
    Serial.printf("\n--- Capture --- Taille: %d octets\n", image_size);

    // 1. PRÉPARATION DU MESSAGE DE DÉBUT (START)
    // Format : START:TAILLE:MAC_ADDRESS
    String mac = WiFi.macAddress();
    String startMsg = "START:" + String(image_size) + ":" + mac;
    
    Serial.print("Envoi Info: ");
    Serial.println(startMsg);
    
    client.publish(TOPIC_INFO, startMsg.c_str());
    
    // Petit délai pour laisser le temps au script Python de traiter le START
    delay(200); 

    // 2. ENVOI DES DONNÉES PAR MORCEAUX (CHUNKS)
    size_t bytes_sent = 0;
    
    while (bytes_sent < image_size) {
      // Important : Maintenir la connexion MQTT active pendant la boucle
      client.loop(); 

      size_t chunk_size = image_size - bytes_sent;
      if (chunk_size > MQTT_CHUNK_SIZE) {
        chunk_size = MQTT_CHUNK_SIZE;
      }
      
      // Envoi du morceau binaire
      if (client.publish(TOPIC_IMAGE, image_buffer + bytes_sent, chunk_size)) {
         // Succès silencieux pour ne pas spammer le port série
      } else {
         Serial.println("Erreur envoi paquet !");
      }
      
      bytes_sent += chunk_size;
      
      // Petit délai pour éviter de saturer le buffer réseau
      delay(20); 
    }

    // 3. MESSAGE DE FIN
    client.publish(TOPIC_INFO, "END");
    Serial.println("Transmission terminee.");
    
    // Libération de la mémoire caméra
    TimerCAM.Camera.free(); 
  }

  // ATTENTE AVANT PROCHAINE PHOTO (10 Secondes)
  // On utilise une boucle pour garder le MQTT "en vie" (ping)
  Serial.println("Attente 10s...");
  for(int i=0; i<100; i++) {
    client.loop();
    delay(100);
  }
}