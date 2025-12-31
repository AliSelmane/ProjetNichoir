#include "M5TimerCAM.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <Preferences.h>

// Bibliothèques système pour la stabilité électrique
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include "Batterie.h"
#include "MiseEnVeille.h"
#include "PrendrePhoto.h"
#include "WifiEtConfig.h"

// Objets globaux
GestionnaireBatterie batterie;
GestionnaireEnergie energie;
GestionnaireWiFi wifi;

const char* mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;
#define MQTT_CHUNK_SIZE 1460 

// --- RÉGLAGE TEMPS DE VEILLE (5 minutes = 300 secondes) ---
const int TEMPS_DODO = 300; 

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  // --- 1. SÉCURITÉ ÉLECTRIQUE IMMÉDIATE ---
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
  pinMode(33, OUTPUT);
  digitalWrite(33, HIGH);     
  gpio_hold_en(GPIO_NUM_33);  

  Serial.begin(115200);
  delay(5000); // Délai de courtoisie au boot
  Serial.println("\n[SYSTEME] Demarrage...");

  wifi.chargerConfig();

  // --- 3. MODE CONFIGURATION (SI VIDE) ---
  if (wifi.ssidLocal == "" || wifi.ssidLocal == "NULL") {
    Serial.println("[CONFIG] Aucun WiFi valide. Ouverture du portail...");
    wifi.lancerConfiguration();
  }

  // --- 5. CONNEXION WIFI ---
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  delay(100);

  if (!wifi.tenterConnexion()) {
    Serial.println("[WIFI] Connexion impossible. Sommeil.");
    energie.dodoProfond(TEMPS_DODO); 
    return;
  }

  wifi.sauvegarderConfig();

  // --- 4. RÉVEIL PAR MOUVEMENT (PIR) ---
  if (!energie.surveillerPIR(50)) {
    Serial.println("[PIR] Pas de mouvement. Retour au dodo.");
    energie.dodoProfond(TEMPS_DODO);
    return;
  }

  // --- 6. INITIALISATION CAMÉRA ---
  TimerCAM.begin();
  energie.allumerFlash(true); 
  delay(500);

  if (TimerCAM.Camera.begin()) {
    TimerCAM.Camera.sensor->set_pixformat(TimerCAM.Camera.sensor, PIXFORMAT_JPEG);
    TimerCAM.Camera.sensor->set_framesize(TimerCAM.Camera.sensor, FRAMESIZE_SVGA);

    // --- 7. CONNEXION MQTT ET ENVOI ---
    client.setServer(mqtt_server, mqtt_port);
    client.setBufferSize(MQTT_CHUNK_SIZE + 512);

    // Utilisation de l'adresse MAC pour l'ID client MQTT (plus stable)
    String mac = WiFi.macAddress();
    String clientId = "Nichoir_" + mac;

    if (client.connect(clientId.c_str())) {
      uint8_t* imgBuf = NULL;
      size_t imgLen = 0;

      if (capturerImage(&imgBuf, &imgLen)) {
        // Envoi batterie
        String battMsg = "BATT;" + batterie.pourcentageToString() + "%";
        client.publish("nichoir/sensor/battery", battMsg.c_str());

        Serial.print(batterie.pourcentageToString()) ;

        // --- ENVOI MAC DANS LE START ---
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

  // --- 8. EXTINCTION PROPRE ---
  energie.allumerFlash(false);

  delay(2000);
  client.disconnect();
  WiFi.disconnect(true);

  Serial.println("[TERMINE] Mise en veille pour 5 minutes.");
  
  // On s'assure que dodoProfond reçoit bien les secondes
  energie.dodoProfond(TEMPS_DODO); 
}

void loop() {}