#include "M5TimerCAM.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include "esp_system.h"
#include "esp_sleep.h"
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

// ===========================================================================
// CONFIGURATION UTILISATEUR
// ===========================================================================
#define PIR_PIN GPIO_NUM_13            // Pin du capteur PIR
#define TIME_TO_SLEEP  3600            // Temps de pause en secondes (1 heure)
#define uS_TO_S_FACTOR 1000000ULL      // Facteur de conversion
#define BURST_COUNT    11              // Nombre de photos par rafale
#define BURST_DELAY    5000            // Intervalle de 5 secondes entre les photos

// ===========================================================================
// VARIABLES GLOBALES
// ===========================================================================
Preferences preferences;
WebServer server(80);
DNSServer dnsServer;
WiFiClient espClient;
PubSubClient client(espClient);

String config_ssid = "";
String config_pass = "";
String config_mqtt = "";
String config_id = "";
String config_loc = "";

bool shouldSaveConfig = false;

// ===========================================================================
// FONCTIONS UTILITAIRES
// ===========================================================================

void ledBlink(int times, int speed) {
    pinMode(2, OUTPUT);
    for (int i = 0; i < times; i++) {
        digitalWrite(2, HIGH); delay(speed);
        digitalWrite(2, LOW);  delay(speed);
    }
}

bool loadConfig() {
    preferences.begin("nichoir-cfg", true);
    config_ssid = preferences.getString("ssid", "");
    config_pass = preferences.getString("password", "");
    config_mqtt = preferences.getString("mqtt", "test.mosquitto.org");
    config_id   = preferences.getString("id", "Nichoir_01");
    config_loc  = preferences.getString("location", "Jardin");
    preferences.end();
    return (config_ssid != "");
}

void saveConfig(String s, String p, String m, String i, String l) {
    preferences.begin("nichoir-cfg", false);
    preferences.putString("ssid", s);
    preferences.putString("password", p);
    preferences.putString("mqtt", m);
    preferences.putString("id", i);
    preferences.putString("location", l);
    preferences.end();
}

void goToSleep(bool enableTimer, bool enablePIR) {
    Serial.println(">>> Passage en veille...");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    if (enableTimer) {
        esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    }
    if (enablePIR) {
        esp_sleep_enable_ext0_wakeup(PIR_PIN, 1);
    }
    Serial.flush();
    esp_deep_sleep_start();
}

// --- VERSION AMÉLIORÉE : BUFFER PLUS GROS ---
bool connectMqtt() {
    client.setServer(config_mqtt.c_str(), 1883);
    // Augmentation drastique du buffer pour éviter les rejets
    client.setBufferSize(10240); 
    
    if (!client.connected()) {
        Serial.print("Connexion MQTT...");
        if (client.connect(config_id.c_str())) {
            Serial.println("OK");
            return true;
        } else {
            Serial.print("Echec rc="); Serial.println(client.state());
            return false;
        }
    }
    return true;
}

// --- VERSION AMÉLIORÉE : LOGIQUE DE RÉESSAI (RETRY) ---
void takeAndSendPhoto(int index) {
    // 1. Prise de la photo
    if (!TimerCAM.Camera.get()) {
        Serial.println("Erreur: Impossible de prendre la photo");
        return;
    }

    size_t imgSize = TimerCAM.Camera.fb->len;
    uint8_t *imgBuf = TimerCAM.Camera.fb->buf;

    // 2. Connexion et Envoi
    if (connectMqtt()) {
        Serial.printf(">>> Envoi photo %d/%d (Taille: %d bytes)\n", index, BURST_COUNT, imgSize);
        
        String infoTopic = "camera/image/info";
        String dataTopic = "camera/image/data";
        
        // Envoi Header
        String infoMsg = "START:" + String(imgSize) + ":" + config_id + ":" + config_loc + ":PHOTO_" + String(index);
        client.publish(infoTopic.c_str(), infoMsg.c_str());

        // Envoi des morceaux (Chunks) avec sécurité
        const int chunkSize = 1024; // Taille d'un morceau
        size_t sent = 0;
        
        while (sent < imgSize) {
            size_t toSend = imgSize - sent;
            if (toSend > chunkSize) toSend = chunkSize;
            
            // TENTATIVE D'ENVOI DU MORCEAU (5 essais max)
            bool chunkSent = false;
            for(int retry=0; retry<5; retry++) {
                if(client.publish(dataTopic.c_str(), imgBuf + sent, toSend)) {
                    chunkSent = true;
                    break; // Succès, on sort de la boucle de retry
                } else {
                    Serial.print("X"); // Marqueur d'erreur
                    delay(50);
                    client.loop(); // Maintenance réseau
                    
                }
            }

            if (!chunkSent) {
                Serial.println("\n!!! ABANDON PHOTO (Erreur MQTT) !!!");
                sent-=1 ;
                break; // On arrête d'envoyer cette photo corrompue
            }

            sent += toSend;
            client.loop(); // Important pour garder la connexion vivante
            delay(20);     // Ralentir un peu pour laisser le broker respirer
        }
        
        client.publish(infoTopic.c_str(), "END");
        Serial.println(" -> Photo envoyée.");
    }
    
    TimerCAM.Camera.free();
}

// ===========================================================================
// CONFIGURATION WEB & PORTAIL CAPTIF
// ===========================================================================
String getHtmlPage() {
    Serial.println("Scan des réseaux WiFi...");
    int n = WiFi.scanNetworks();
    Serial.println("Scan terminé");

    String options = "";
    if (n == 0) {
        options = "<option value=''>Aucun réseau trouvé</option>";
    } else {
        for (int i = 0; i < n; ++i) {
            options += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + " (" + WiFi.RSSI(i) + " dBm)</option>";
        }
    }

    String html = R"rawliteral(<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width, initial-scale=1"><title>Config Nichoir</title><style>body{font-family:sans-serif;text-align:center;padding:20px;background:#f4f4f4;}form{background:#fff;padding:20px;border-radius:10px;box-shadow:0 0 10px rgba(0,0,0,0.1);max-width:400px;margin:0 auto;}h2{margin-top:0;}input,select{width:100%;padding:12px;margin:8px 0;border:1px solid #ddd;border-radius:4px;box-sizing:border-box;}input[type=submit]{background:#007bff;color:white;font-weight:bold;border:none;cursor:pointer;}input[type=submit]:hover{background:#0056b3;}</style></head><body><form action="/save" method="POST"><h2>Configuration Nichoir</h2>)rawliteral";
    html += "<label>Choisir WiFi:</label><select name='ssid'>" + options + "</select>";
    html += "<label>Mot de passe:</label><input type='password' name='password' placeholder='Mot de passe'>";
    html += "<label>Broker MQTT:</label><input type='text' name='mqtt' value='" + config_mqtt + "'>";
    html += "<label>ID Appareil:</label><input type='text' name='deviceid' value='" + config_id + "'>";
    html += "<label>Localisation:</label><input type='text' name='location' value='" + config_loc + "'>";
    html += "<input type='submit' value='ENREGISTRER'></form></body></html>";
    return html;
}

void startConfigMode() {
    Serial.println(">>> Mode Configuration (Portail Captif)");
    
    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP_STA); // Mode AP + Station pour permettre le scan
    WiFi.softAP("Nichoir_Config", ""); 
    
    delay(500);
    IPAddress apIP = WiFi.softAPIP();
    Serial.print("IP AP: "); Serial.println(apIP);

    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(53, "*", apIP);

    server.on("/", []() {
        server.send(200, "text/html", getHtmlPage());
    });

    server.onNotFound([]() {
        server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/", true);
        server.send(302, "text/plain", "");
    });

    server.on("/save", []() {
        String s = server.arg("ssid");
        String p = server.arg("password");
        String m = server.arg("mqtt");
        String i = server.arg("deviceid");
        String l = server.arg("location");

        if (s.length() > 0) {
            saveConfig(s, p, m, i, l);
            server.send(200, "text/html", "<h1>Sauvegarde OK!</h1><p>Redemarrage en mode surveillance...</p>");
            delay(2000);
            shouldSaveConfig = true;
        } else {
            server.send(200, "text/html", "SSID Manquant <a href='/'>Retour</a>");
        }
    });

    server.begin();
    
    while (!shouldSaveConfig) {
        dnsServer.processNextRequest(); 
        server.handleClient();          
        ledBlink(1, 50); 
        delay(10);
    }

    Serial.println("Config terminee.");
    ledBlink(3, 200);
    goToSleep(false, true); 
}

// ===========================================================================
// SETUP PRINCIPAL
// ===========================================================================
void setup() {
    Serial.begin(115200);
    digitalWrite(33,HIGH);
    TimerCAM.begin(true); 
    pinMode(2, OUTPUT);   

    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    // --- CAS 1 : REVEIL SUR MOUVEMENT ---
    if (cause == ESP_SLEEP_WAKEUP_EXT0) {
        Serial.println(">>> REVEIL PIR");
        if (loadConfig()) {
            WiFi.begin(config_ssid.c_str(), config_pass.c_str());
            
            int attempts = 0;
            while (WiFi.status() != WL_CONNECTED && attempts < 20) {
                delay(500); attempts++;
            }

            if (WiFi.status() == WL_CONNECTED) {
                digitalWrite(2, HIGH); 
                TimerCAM.Camera.begin();
                TimerCAM.Camera.sensor->set_pixformat(TimerCAM.Camera.sensor, PIXFORMAT_JPEG);
                TimerCAM.Camera.sensor->set_framesize(TimerCAM.Camera.sensor, FRAMESIZE_VGA);

                for (int i = 1; i <= BURST_COUNT; i++) {
                    takeAndSendPhoto(i);
                    // Si on perd la connexion MQTT entre les photos, on tente de reconnecter
                    if (!client.connected()) connectMqtt();
                    client.loop(); // Maintenance socket
                    
                    if (i < BURST_COUNT) delay(BURST_DELAY); 
                }
                digitalWrite(2, LOW);
            }
        }
        goToSleep(true, false); 
    }

    // --- CAS 2 : REVEIL TIMER (FIN DE PAUSE) ---
    else if (cause == ESP_SLEEP_WAKEUP_TIMER) {
        Serial.println(">>> FIN PAUSE");
        goToSleep(false, true); 
    }

    // --- CAS 3 : BOUTON RESET (MODE CONFIG) ---
    else {
        Serial.println(">>> RESET -> CONFIG");
        loadConfig(); 
        startConfigMode(); 
    }
}

void loop() {
    // Vide
}