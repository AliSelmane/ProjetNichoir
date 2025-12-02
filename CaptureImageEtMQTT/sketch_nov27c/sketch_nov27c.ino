#include "M5TimerCAM.h"
#include <WiFi.h>
#include <PubSubClient.h>

// Configuration WiFi
const char* ssid = "electroProjectWifi";
const char* password = "B1MesureEnv";

// Configuration MQTT
const char* mqtt_server = "test.mosquitto.org"; // Ex: "test.mosquitto.org"
const int mqtt_port = 1883;
const char* mqtt_client_id = "ESP32_TimerCAM";
const char* TOPIC_IMAGE = "camera/image/data"; // Topic pour les données binaires
const char* TOPIC_INFO = "camera/image/info";  // Topic pour signaler le début/fin

WiFiClient espClient;
PubSubClient client(espClient);

// Taille maximale d'un paquet MQTT (ajustez selon votre broker)
// 1024 octets est une taille sûre pour la plupart des brokers.
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
  Serial.print("Adresse IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Boucle jusqu'a ce que nous soyons reconnectes
  while (!client.connected()) {
    Serial.print("Tentative de connexion MQTT...");
    // Tente de se connecter
    if (client.connect(mqtt_client_id)) {
      Serial.println("connecte!");
      // On ne souscrit a rien ici, on ne fait que publier
    } else {
      Serial.print("echec, rc=");
      Serial.print(client.state());
      Serial.println(" nouvelle tentative dans 5 secondes");
      delay(5000);
    }
  }
}

// --- Configuration et Envoi ---

void setup() {
  Serial.begin(115200);
  setup_wifi();
  
  client.setServer(mqtt_server, mqtt_port);

  // Initialisation de la camera (comme votre code original)
  TimerCAM.begin();
  if (!TimerCAM.Camera.begin()) {
      Serial.println("Camera Init Fail");
      return;
  }
  Serial.println("Camera Init Success");

  TimerCAM.Camera.sensor->set_pixformat(TimerCAM.Camera.sensor, PIXFORMAT_JPEG);
  TimerCAM.Camera.sensor->set_framesize(TimerCAM.Camera.sensor, FRAMESIZE_UXGA);
  TimerCAM.Camera.sensor->set_vflip(TimerCAM.Camera.sensor, 1);
  TimerCAM.Camera.sensor->set_hmirror(TimerCAM.Camera.sensor, 0);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); // Traitement du client MQTT

  // Tente de capturer une image
  if (TimerCAM.Camera.get()) {
    size_t image_size = TimerCAM.Camera.fb->len;
    uint8_t *image_buffer = TimerCAM.Camera.fb->buf;
    
    Serial.printf("Image capturee. Taille: %d octets.\n", image_size);

    // 1. SIGNALER LE DEBUT DE LA TRANSMISSION
    // Publier la taille totale de l'image (pour aider le cote Python)
    char info_msg[50];
    sprintf(info_msg, "START:%d", image_size);
    client.publish(TOPIC_INFO, info_msg);
    delay(50); // Petit delai pour s'assurer que le broker a traite l'info

    // 2. ENVOI DES DONNEES PAR MORCEAUX (Chunks)
    size_t bytes_sent = 0;
    while (bytes_sent < image_size) {
      // Determine la taille du paquet actuel
      size_t chunk_size = image_size - bytes_sent;
      if (chunk_size > MQTT_CHUNK_SIZE) {
        chunk_size = MQTT_CHUNK_SIZE;
      }
      
      // Publie le morceau
      // L'argument retain=false est essentiel ici. QoS=0 est suffisant.
      client.publish(TOPIC_IMAGE, image_buffer + bytes_sent, chunk_size);
      
      bytes_sent += chunk_size;
      Serial.printf("Envoye: %d / %d octets.\n", bytes_sent, image_size);
      delay(5000); // Petit delai pour eviter de surcharger le broker
    }

    // 3. SIGNALER LA FIN DE LA TRANSMISSION
    client.publish(TOPIC_INFO, "END");
    Serial.println("Transmission terminee.");
    
    TimerCAM.Camera.free(); // Liberer le tampon memoire
  }

  delay(10000); // Prendre une nouvelle photo toutes les 10 secondes
}