#include <WiFi.h>
#include <PubSubClient.h>

#define SLEEP_TIME 10  

const char* ssid = "electroProjectWifi";
const char* password = "B1MesureEnv";

const char* mqtt_server = "test.mosquitto.org"; 
const int mqtt_port = 1883;

const char* topic_pub = "timerCam/test";
const char* topic_sub = "timerCam/cmd";


WiFiClient espClient;
PubSubClient client(espClient);

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("[MQTT] Message reçu sur ");
  Serial.print(topic);
  Serial.print(" : ");

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

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

}

void reconnect_mqtt() {

  while (!client.connected()) {
    Serial.print("[MQTT] Connexion au broker… ");

    if (client.connect("TimerCamClient")) {
      Serial.println("OK !");

      client.subscribe(topic_sub);
      Serial.print("[MQTT] Abonné à : ");
      Serial.println(topic_sub);

      client.publish(topic_pub, "TimerCam connectée !");
    } else {
      Serial.print("ÉCHEC, code = ");
      Serial.println(client.state());
      Serial.println("Nouvel essai dans 3 secondes…");
      delay(3000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqtt_callback);
}
void loop() {

  if (!client.connected()) {
    reconnect_mqtt();
  }

  client.loop();

  static unsigned long lastSend = 0;
  static int number = 0;  

  if (millis() - lastSend > 5000) {
    lastSend = millis();

    String numberStr = String(number);

    client.publish(topic_pub, numberStr.c_str());

    Serial.print("[MQTT] Nombre publié : ");
    Serial.println(number);


    number++;
  }
}
