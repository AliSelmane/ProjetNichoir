#include <WiFi.h>
#include <PubSubClient.h>

#define SLEEP_TIME 10  // Temps en secondes (ici 10 secondes)
// ----------------------------------------------------
// CONFIG WIFI
// ----------------------------------------------------
const char* ssid = "electroProjectWifi";
const char* password = "B1MesureEnv";

// ----------------------------------------------------
// CONFIG MQTT
// ----------------------------------------------------
const char* mqtt_server = "test.mosquitto.org";  // broker gratuit pour test
const int mqtt_port = 1883;

// Topic de test
const char* topic_pub = "timerCam/test";
const char* topic_sub = "timerCam/cmd";


// ----------------------------------------------------
// OBJECTS
// ----------------------------------------------------
WiFiClient espClient;
PubSubClient client(espClient);

// ----------------------------------------------------
// CALLBACK MQTT: quand on reçoit un message sur un topic
// ----------------------------------------------------
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("[MQTT] Message reçu sur ");
  Serial.print(topic);
  Serial.print(" : ");

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

// ----------------------------------------------------
// Connexion au WiFi
// ----------------------------------------------------
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.println("[WIFI] Connexion au réseau…");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n[WIFI] CONNECTÉ !");
  Serial.print("[WIFI] IP = ");
  Serial.println(WiFi.localIP());

 // esp_sleep_enable_timer_wakeup(SLEEP_TIME * 1000000);  // en microsecondes
}

// ----------------------------------------------------
// Connexion au broker MQTT
// ----------------------------------------------------
void reconnect_mqtt() {
  // Boucle jusqu'à ce que ce soit connecté
  while (!client.connected()) {
    Serial.print("[MQTT] Connexion au broker… ");

    if (client.connect("TimerCamClient")) {
      Serial.println("OK !");

      // S'abonner
      client.subscribe(topic_sub);
      Serial.print("[MQTT] Abonné à : ");
      Serial.println(topic_sub);

      // Publier un message de présence
      client.publish(topic_pub, "TimerCam connectée !");
    } else {
      Serial.print("ÉCHEC, code = ");
      Serial.println(client.state());
      Serial.println("Nouvel essai dans 3 secondes…");
      delay(3000);
    }
  }
}

// ----------------------------------------------------
// SETUP
// ----------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(1000);

  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqtt_callback);
}

// ----------------------------------------------------
// LOOP
// ----------------------------------------------------
void loop() {
  // reconnecter si déconnecté
  if (!client.connected()) {
    reconnect_mqtt();
  }

  client.loop();

  // Publier un nombre qui s'incrémente toutes les 5 secondes
  static unsigned long lastSend = 0;
  static int number = 0;  // Nombre qui va s'incrémenter

  if (millis() - lastSend > 5000) {
    lastSend = millis();

    // Convertir le nombre en chaîne de caractères
    String numberStr = String(number);

    // Publier le nombre
    client.publish(topic_pub, numberStr.c_str());

    // Afficher le nombre dans le moniteur série
    Serial.print("[MQTT] Nombre publié : ");
    Serial.println(number);


    // Incrémenter le nombre
    number++;
  }
}
