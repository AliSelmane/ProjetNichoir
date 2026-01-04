#include "M5TimerCAM.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include "esp_system.h"

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include "Batterie.h"
#include "MiseEnVeille.h"
#include "PrendrePhoto.h"
#include "WifiEtConfig.h"

GBatterie batterie;
GEnergie energie;
GWifi wifi;

const char* mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;
#define MQTT_CHUNK_SIZE 1460

const int TEMPVEILLE = 300;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // ça permet de désactiver les protections qui coupe l'esp si la tension chute trop bas
  pinMode(2, OUTPUT);

  pinMode(33, OUTPUT);
  digitalWrite(33, HIGH);
  gpio_hold_en(GPIO_NUM_33);  // ça verouille la pine 33 a l'état haut

  Serial.begin(115200);
  delay(5000);
  Serial.println("\n[SYSTEME] Demarrage...");

  wifi.chargerConfig();

  // MODE CONFIGURATION
  if (wifi.ssidLocal == "" || wifi.ssidLocal == "NULL") {
    Serial.println("[CONFIG] Aucun WiFi valide. Ouverture du portail...");
    wifi.lancerConfiguration();
  }

  // ---  CONNEXION WIFI ---
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  delay(100);

  if (!wifi.tenterConnexion()) {
    Serial.println("[WIFI] Connexion impossible. Sommeil.");
    energie.SleepProfond(TEMPVEILLE);
    return;
  }

  wifi.sauvegarderConfig();

  delay(1000) ;

  // 4. RÉVEIL PAR MOUVEMENT
  if (!energie.surveillerPIR(60)) {
    Serial.println("[PIR] Pas de mouvement. Retour au dodo.");
    energie.SleepProfond(TEMPVEILLE);
    return;
  }

  // INITIALISATION CAMÉRA ---
  TimerCAM.begin();
  energie.allumerFlash(true);

  delay(2000);

  if (TimerCAM.Camera.begin()) {
    digitalWrite(2, HIGH);

    TimerCAM.Camera.sensor->set_pixformat(TimerCAM.Camera.sensor, PIXFORMAT_JPEG);  // défènis le type d'image en JPEG
    TimerCAM.Camera.sensor->set_framesize(TimerCAM.Camera.sensor, FRAMESIZE_SVGA);  // la résolution 800x600
    digitalWrite(2, HIGH);

    // CONNEXION MQTT
    client.setServer(mqtt_server, mqtt_port);
    client.setBufferSize(MQTT_CHUNK_SIZE + 512);

    String mac = WiFi.macAddress();
    String clientId = "Nichoir_" + mac;

    if (client.connect(clientId.c_str())) {
      uint8_t* imgBuf = NULL;
      size_t imgLen = 0;

      if (capturerImage(&imgBuf, &imgLen)) {
        // Envoi batterie
        String battMsg = "BATT;" + batterie.pourcentageToString() + "%";
        client.publish("nichoir/sensor/battery", battMsg.c_str());  //convertis le pourcentage en string

        Serial.print(batterie.pourcentageToString());

        // ENVOI MAC
        // Format; START;ADRESSE_MAC;NOM;LIEU
        String startMsg = "START;" + mac + ";" + wifi.nomNichoir + ";" + wifi.localisation;
        client.publish("nichoir/camera/info", startMsg.c_str());
        delay(100);

        // Envoi image par morceaux
        size_t env = 0;
        while (env < imgLen) {
          size_t t = min((size_t)MQTT_CHUNK_SIZE, imgLen - env);
          if (client.publish("nichoir/camera/data", imgBuf + env, t)) {
            env += t;
            Serial.print(".");
          }
          client.loop();
          delay(20);
        }

        Serial.println("\n[MQTT] Envoi END.");
        client.publish("nichoir/camera/info", "END");
        delay(500);
      }
    }
  }
  energie.allumerFlash(false);

  delay(2000);
  client.disconnect();
  WiFi.disconnect(true);

  Serial.println("[TERMINE] Mise en veille pour 5 minutes.");

  energie.SleepProfond(TEMPVEILLE);
}

void loop() {

  energie.SleepProfond(TEMPVEILLE);
}