#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>


const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
DNSServer dnsServer;
WebServer server(80);
Preferences preferences;

String config_ssid = "";
String config_pass = "";
String config_mqtt = "test.mosquitto.org";
String config_id = "Nichoir_01";
String config_location = "Jardin";

bool isConfigMode = false;


String getPageHTML() {
  int n = WiFi.scanNetworks();
  String options = "";
  for (int i = 0; i < n; ++i) {
    options += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + " (" + WiFi.RSSI(i) + " dBm)</option>";
  }

  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="UTF-8">
  <title>Config Nichoir</title>
  <style>
    body { font-family: Arial, sans-serif; background-color: #f2f2f2; text-align: center; padding: 20px; }
    form { background-color: white; max-width: 400px; margin: auto; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }
    h1 { color: #2c3e50; }
    input, select { width: 100%; padding: 10px; margin: 8px 0; border: 1px solid #ccc; border-radius: 4px; box-sizing: border-box; }
    input[type=submit] { background-color: #4CAF50; color: white; border: none; cursor: pointer; font-size: 16px; font-weight: bold; }
    input[type=submit]:hover { background-color: #45a049; }
    .label { text-align: left; font-weight: bold; margin-top: 10px; display: block; }
  </style>
</head>
<body>
  <h1>🐦 Configuration Nichoir</h1>
  <form action="/save" method="POST">
    
    <label class="label">Choisir le WiFi :</label>
    <select name="ssid">)rawliteral";

  html += options;

  html += R"rawliteral(
    </select>

    <label class="label">Mot de passe WiFi :</label>
    <input type="password" name="password" placeholder="Mot de passe...">

    <hr>

    <label class="label">Adresse Broker MQTT :</label>
    <input type="text" name="mqtt" value=")rawliteral"
          + config_mqtt + R"rawliteral(">

    <label class="label">Identifiant Unique (ID) :</label>
    <input type="text" name="deviceid" value=")rawliteral"
          + config_id + R"rawliteral(">

    <label class="label">Localisation (ex: Jardin, Forêt) :</label>
    <input type="text" name="location" value=")rawliteral"
          + config_location + R"rawliteral(">

    <br><br>
    <input type="submit" value="ENREGISTRER & REDÉMARRER">
  </form>
</body>
</html>
)rawliteral";

  return html;
}


void handleRoot() {
  server.send(200, "text/html", getPageHTML());
}

void handleSave() {

  String s_ssid = server.arg("ssid");
  String s_pass = server.arg("password");
  String s_mqtt = server.arg("mqtt");
  String s_id = server.arg("deviceid");
  String s_loc = server.arg("location");

  if (s_ssid.length() > 0) {
    preferences.begin("nichoir-cfg", false);
    preferences.putString("ssid", s_ssid);
    preferences.putString("password", s_pass);
    preferences.putString("mqtt", s_mqtt);
    preferences.putString("id", s_id);
    preferences.putString("location", s_loc);
    preferences.end();

    String message = "<h1>Sauvegarde OK !</h1><p>L'ESP32 va redemarrer et se connecter a " + s_ssid + ".</p>";
    server.send(200, "text/html", message);

    delay(2000);
    ESP.restart();
  } else {
    server.send(200, "text/plain", "Erreur: Le SSID est vide.");
  }
}

void handleNotFound() {

  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

void setupConfigMode() {
  isConfigMode = true;
  WiFi.mode(WIFI_AP_STA);

  WiFi.softAP("Nichoir_Configuration", "");

  delay(100);

  dnsServer.start(DNS_PORT, "*", apIP);

  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.onNotFound(handleNotFound);
  server.begin();

  Serial.println(">> MODE CONFIGURATION ACTIVÉ");
  Serial.println("Connectez-vous au WiFi 'Nichoir_Configuration'");
  Serial.println("Et allez sur http://192.168.4.1");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  preferences.begin("nichoir-cfg", true);
  config_ssid = preferences.getString("ssid", "");
  config_pass = preferences.getString("password", "");
  config_mqtt = preferences.getString("mqtt", "test.mosquitto.org");
  config_id = preferences.getString("id", "Nichoir_Unknown");
  config_location = preferences.getString("location", "Non_Defini");
  preferences.end();

  Serial.println("Config chargee :");
  Serial.println("SSID: " + config_ssid);

  if (config_ssid == "") {
    setupConfigMode();
  } else {
    Serial.print("Tentative de connexion a ");
    Serial.println(config_ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(config_ssid.c_str(), config_pass.c_str());

    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 40) {
      delay(500);
      Serial.print(".");
      retries++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnecté au WiFi !");
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
      Serial.print("Localisation: ");
      Serial.println(config_location);
    } else {
      Serial.println("\nEchec connexion WiFi. Lancement mode Config.");
      setupConfigMode();
    }
  }
}


void loop() {
  if (isConfigMode) {
    dnsServer.processNextRequest();
    server.handleClient();
  } else {

    static unsigned long lastTime = 0;
    if (millis() - lastTime > 5000) {
      lastTime = millis();
      Serial.println("Je suis en mode normal ! Localisation: " + config_location);
    }
  }
}