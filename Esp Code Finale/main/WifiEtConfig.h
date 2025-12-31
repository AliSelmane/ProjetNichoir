#include "M5TimerCAM.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

class GestionnaireWiFi {
public:
  WebServer serveur;
  DNSServer dns;
  String ssidLocal, passeLocal, nomNichoir, localisation;
  bool donneesRecues = false;

  String genererPage() {
    Serial.println("[WIFI] Scan des réseaux en cours...");
    int n = WiFi.scanNetworks();
    Serial.printf("[WIFI] %d réseaux trouvés.\n", n);

    String options = "";
    String reseauxVus[n];
    int nbUnique = 0;

    for (int i = 0; i < n; ++i) {
      bool doublon = false;
      for (int j = 0; j < nbUnique; j++) {
        if (WiFi.SSID(i) == reseauxVus[j]) {
          doublon = true;
          break;
        }
      }
      if (!doublon) {
        reseauxVus[nbUnique] = WiFi.SSID(i);
        options += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</option>";
        nbUnique++;
      }
    }

    String html = R"rawliteral(<!DOCTYPE html><html><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1">
        <style>body{font-family:sans-serif;text-align:center;padding:20px;background:#f4f4f4;}
        form{background:#fff;padding:20px;border-radius:10px;max-width:400px;margin:0 auto;}
        input,select{width:100%;padding:12px;margin:8px 0;border:1px solid #ddd;border-radius:4px;box-sizing:border-box;}
        input[type=submit]{background:#007bff;color:white;font-weight:bold;border:none;cursor:pointer;}</style></head>
        <body><form action="/save" method="POST"><h2>Configuration Nichoir</h2>)rawliteral";
    html += "<label>WiFi :</label><select name='s'>" + options + "</select>";
    html += "<input type='password' name='p' placeholder='Mot de passe'>";
    html += "<input type='text' name='n' placeholder='Nom du Nichoir'>";
    html += "<input type='text' name='l' placeholder='Localisation'>";
    html += "<input type='submit' value='CONNEXION'></form></body></html>";
    return html;
  }

public:
  GestionnaireWiFi()
    : serveur(80) {}


  void lancerConfiguration() {
    donneesRecues = false;
    Serial.println("----------------------------------------------");
    Serial.println("[MODE] Activation du mode Configuration");

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("Nichoir_Config");
    Serial.print("[AP] Réseau créé : Nichoir_Config | IP : ");
    Serial.println(WiFi.softAPIP());

    dns.start(53, "*", WiFi.softAPIP());

    serveur.on("/", [this]() {
      Serial.println("[WEB] Consultation de la page de config.");
      serveur.send(200, "text/html", genererPage());
    });

    serveur.on("/save", [this]() {
      ssidLocal = serveur.arg("s");
      passeLocal = serveur.arg("p");
      nomNichoir = serveur.arg("n");
      localisation = serveur.arg("l");
      Serial.println("[WEB] Formulaire validé.");
      serveur.send(200, "text/html", "Tentative... Vérifiez le moniteur série.");
      donneesRecues = true;
    });

    serveur.onNotFound([this]() {
      serveur.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/", true);
      serveur.send(302, "text/plain", "");
    });

    serveur.begin();
    Serial.println("[INFO] En attente de l'utilisateur...");

    while (!donneesRecues) {
      dns.processNextRequest();
      serveur.handleClient();
      delay(10);
    }

    tenterConnexion();
  }

  bool tenterConnexion() {
    Serial.println("----------------------------------------------");
    Serial.printf("[WIFI] Tentative de connexion à : %s\n", ssidLocal.c_str());
    WiFi.begin(ssidLocal.c_str(), passeLocal.c_str());

    unsigned long debut = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - debut < 30000) {
      delay(500);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n[SUCCÈS] WiFi Connecté !");
      Serial.printf("[INFO] IP obtenue : %s\n", WiFi.localIP().toString().c_str());
      Serial.printf("[DATA] Nom : %s | Lieu : %s\n", nomNichoir.c_str(), localisation.c_str());
      WiFi.softAPdisconnect(true);
      Serial.println("[AP] Point d'accès désactivé.");
      Serial.println("----------------------------------------------");

      return true;
    } else {
      Serial.println("\n[ERREUR] Délai de 10s dépassé. Identifiants incorrects ?");
      lancerConfiguration();

      return false;
    }
  }

  void sauvegarderConfig() {
    Preferences prefs;
    prefs.begin("n-cfg", false);  // "n-cfg" est le nom du namespace

    prefs.putString("s", ssidLocal);
    prefs.putString("p", passeLocal);
    prefs.putString("n", nomNichoir);
    prefs.putString("l", localisation);

    prefs.end();
    Serial.println("[MEMOIRE] Données enregistrées dans la Flash.");
  }

  bool chargerConfig() {
    Preferences prefs;
    prefs.begin("n-cfg", true);  // lecture seule

    ssidLocal = prefs.getString("s", "");
    passeLocal = prefs.getString("p", "");
    nomNichoir = prefs.getString("n", "Nichoir_Inconnu");
    localisation = prefs.getString("l", "Inconnue");

    prefs.end();

    Serial.println("[MEMOIRE] Chargement configuration...");
    Serial.printf("  SSID : %s\n", ssidLocal.c_str());
    Serial.printf("  Nom  : %s\n", nomNichoir.c_str());
    Serial.printf("  Lieu : %s\n", localisation.c_str());

    // Vérification simple de validité
    if (ssidLocal.length() < 2 || passeLocal.length() < 2) {
      Serial.println("[MEMOIRE] Configuration invalide ou absente.");
      return false;
    }

    Serial.println("[MEMOIRE] Configuration valide.");
    return true;
  }
};
