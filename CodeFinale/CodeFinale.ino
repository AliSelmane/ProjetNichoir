/**************************************************************
 * NICHoir connecté - M5Stack TimerCamera (U082 - OV3660)
 * Objectif: autonomie MAX sans registres:
 *  - Deep-sleep agressif
 *  - Wi-Fi OFF dès que possible
 *  - Scan Wi-Fi uniquement en mode configuration (AP)
 *  - Photo quotidienne (toutes les 24h)
 *  - PIR: photo max toutes les 5 min (cooldown en désactivant le réveil PIR)
 *  - Flash uniquement la nuit + si batterie OK
 *
 * Portail AP:
 *  - SSID: Nichoir-XXXXXXXXXXXX
 *  - Mot de passe: nichoir123
 *  - IP: http://192.168.4.1
 **************************************************************/

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <esp_camera.h>
#include <time.h>
#include <esp_sleep.h>
#include <driver/gpio.h>

// ===================== RÉGLAGES AUTONOMIE =====================
static const uint32_t WIFI_TIMEOUT_MS      = 12000;         // connexion rapide
static const uint32_t AP_DUREE_MAX_MS      = 10 * 60 * 1000; // 10 min de portail puis dodo (autonomie)
static const uint32_t SOMMEIL_AP_ECHEC_S   = 30 * 60;       // 30 min avant de retenter (si personne configure)

static const uint32_t INTERVALLE_JOUR_S    = 24 * 60 * 60;  // 24h
static const uint32_t COOLDOWN_PIR_S       = 5 * 60;        // 5 min

static const uint32_t NTP_REFRESH_S        = 7 * 24 * 60 * 60; // resync NTP 1x / semaine
static const char*    TZ_INFO             = "CET-1CEST,M3.5.0/2,M10.5.0/3"; // Europe/Bruxelles
static const char*    NTP1                = "pool.ntp.org";
static const char*    NTP2                = "time.google.com";

// Flash heure (nuit)
static const int HEURE_FLASH_ON_DEBUT = 20; // 20:00
static const int HEURE_FLASH_ON_FIN   = 7;  // jusqu'à 07:00
static const float VBATT_MIN_FLASH    = 3.55f; // en dessous: pas de flash (économie)

// Wi-Fi puissance TX (réduit conso, utile si routeur proche)
static const wifi_power_t WIFI_PUISSANCE_TX = WIFI_POWER_8_5dBm;

// ===================== PINS (M5Stack TimerCamera U082) =====================
// Camera pinmap (source M5 docs / repo TimerCam-idf)
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    15
#define XCLK_GPIO_NUM     27
#define SIOD_GPIO_NUM     25
#define SIOC_GPIO_NUM     23
#define Y9_GPIO_NUM       19
#define Y8_GPIO_NUM       36
#define Y7_GPIO_NUM       18
#define Y6_GPIO_NUM       39
#define Y5_GPIO_NUM       5
#define Y4_GPIO_NUM       34
#define Y3_GPIO_NUM       35
#define Y2_GPIO_NUM       32
#define VSYNC_GPIO_NUM    22
#define HREF_GPIO_NUM     26
#define PCLK_GPIO_NUM     21

// LED carte (petite LED d’état, peut servir de "mini flash" si tu veux)
static const int PIN_LED_CARTE = 2;

// Connecteur HY2.0-4P (sur TimerCamera): G13 / G4 / 5V / GND
// Pour réveil deep-sleep EXT0: choisir une broche RTC.
// GPIO13 et GPIO4 sont RTC sur ESP32 -> OK. On met PIR sur 13 par défaut :
static const int PIN_PIR = 13;

// Batterie (TimerCamera): BAT_ADC_Pin = G38
static const int PIN_BAT_ADC = 38;
// ⚠️ L’entrée batterie dépend du montage interne (pont diviseur, calibration).
// On garde une lecture simple (suffisante pour décider flash oui/non).
static const float COEF_BAT_APPROX = 2.0f; // adapte si tes valeurs sont fausses
static const float VREF_APPROX     = 1.1f; // approx

// ===================== PORTAIL AP =====================
static const char* AP_PASSWORD = "nichoir123";

// ===================== CONFIG RPI (valeurs défaut) =====================
static const int   RPI_PORT_DEFAUT = 5000;
static const char* RPI_PATH_DEFAUT = "/upload";

// ===================== STOCKAGE =====================
Preferences prefs;
WebServer server(80);
DNSServer dnsServer;

// RTC mémoire (persiste pendant deep-sleep)
RTC_DATA_ATTR uint32_t prochain_jour_epoch = 0;
RTC_DATA_ATTR bool     cooldown_pir_actif  = false;
RTC_DATA_ATTR uint32_t cooldown_fin_epoch  = 0;
RTC_DATA_ATTR uint32_t dernier_ntp_epoch   = 0;

// Config persistante
String cfg_ssid, cfg_pass, cfg_rpi_host, cfg_rpi_path;
int    cfg_rpi_port = RPI_PORT_DEFAUT;

// ===================== OUTILS =====================
String idAppareil() {
  uint64_t mac = ESP.getEfuseMac();
  char buf[13];
  snprintf(buf, sizeof(buf), "%04X%08X", (uint16_t)(mac >> 32), (uint32_t)mac);
  return String(buf);
}

uint32_t epochActuel() {
  time_t now;
  time(&now);
  return (uint32_t)now;
}

bool heureValide() {
  return epochActuel() > 1577836800UL; // > 2020-01-01
}

int heureLocale() {
  struct tm t;
  time_t now;
  time(&now);
  localtime_r(&now, &t);
  return t.tm_hour;
}

float lireBatterieVoltsApprox() {
  analogReadResolution(12);
  uint16_t raw = analogRead(PIN_BAT_ADC);
  float v = (raw / 4095.0f) * VREF_APPROX;
  v *= COEF_BAT_APPROX;
  return v;
}

bool flashDoitEtreActive(float vbatt) {
  if (!heureValide()) return false;       // si pas d’heure: on évite
  if (vbatt < VBATT_MIN_FLASH) return false;

  int h = heureLocale();
  return (h >= HEURE_FLASH_ON_DEBUT) || (h < HEURE_FLASH_ON_FIN);
}

// ===================== CAMERA =====================
bool initialiserCamera(framesize_t taille, int qualite) {
  camera_config_t c;
  c.ledc_channel = LEDC_CHANNEL_0;
  c.ledc_timer   = LEDC_TIMER_0;
  c.pin_d0       = Y2_GPIO_NUM;
  c.pin_d1       = Y3_GPIO_NUM;
  c.pin_d2       = Y4_GPIO_NUM;
  c.pin_d3       = Y5_GPIO_NUM;
  c.pin_d4       = Y6_GPIO_NUM;
  c.pin_d5       = Y7_GPIO_NUM;
  c.pin_d6       = Y8_GPIO_NUM;
  c.pin_d7       = Y9_GPIO_NUM;
  c.pin_xclk     = XCLK_GPIO_NUM;
  c.pin_pclk     = PCLK_GPIO_NUM;
  c.pin_vsync    = VSYNC_GPIO_NUM;
  c.pin_href     = HREF_GPIO_NUM;
  c.pin_sccb_sda = SIOD_GPIO_NUM;
  c.pin_sccb_scl = SIOC_GPIO_NUM;
  c.pin_pwdn     = PWDN_GPIO_NUM;
  c.pin_reset    = RESET_GPIO_NUM;

  c.xclk_freq_hz = 20000000;
  c.pixel_format = PIXFORMAT_JPEG;

  c.frame_size   = taille;   // QVGA/VGA...
  c.jpeg_quality = qualite;  // 10-15: plus grand = plus compressé
  c.fb_count     = 1;

  if (psramFound()) c.fb_location = CAMERA_FB_IN_PSRAM;

  return esp_camera_init(&c) == ESP_OK;
}

camera_fb_t* capturerImage(bool flashLed) {
  pinMode(PIN_LED_CARTE, OUTPUT);
  digitalWrite(PIN_LED_CARTE, LOW);

  if (flashLed) {
    digitalWrite(PIN_LED_CARTE, HIGH);
    delay(120);
  }

  camera_fb_t* fb = esp_camera_fb_get();

  if (flashLed) digitalWrite(PIN_LED_CARTE, LOW);
  return fb;
}

// ===================== ENVOI AU RPI (HTTP POST brut) =====================
bool envoyerAuRpi(camera_fb_t* fb, const String& raison, float vbatt, bool flash) {
  if (!fb || fb->len == 0) return false;

  WiFiClient client;
  client.setTimeout(4000);

  if (!client.connect(cfg_rpi_host.c_str(), cfg_rpi_port)) return false;

  uint32_t ep = epochActuel();
  String path = cfg_rpi_path;
  if (!path.startsWith("/")) path = "/" + path;

  // Requête HTTP minimale (économe) + méta via headers
  client.print(String("POST ") + path + " HTTP/1.1\r\n");
  client.print(String("Host: ") + cfg_rpi_host + "\r\n");
  client.print("Connection: close\r\n");
  client.print("Content-Type: image/jpeg\r\n");
  client.print(String("Content-Length: ") + fb->len + "\r\n");
  client.print(String("X-Chip: ") + idAppareil() + "\r\n");
  client.print(String("X-Epoch: ") + ep + "\r\n");
  client.print(String("X-Vbatt: ") + String(vbatt, 2) + "\r\n");
  client.print(String("X-Raison: ") + raison + "\r\n");
  client.print(String("X-Flash: ") + (flash ? "1" : "0") + "\r\n");
  client.print("\r\n");

  // Corps : JPEG
  client.write(fb->buf, fb->len);

  // Lire juste le status line (optionnel)
  uint32_t t0 = millis();
  while (!client.available() && (millis() - t0) < 2500) delay(10);

  bool ok = false;
  if (client.available()) {
    String line = client.readStringUntil('\n');
    // Exemple: HTTP/1.1 200 OK
    ok = line.indexOf("200") >= 0 || line.indexOf("201") >= 0;
  }

  client.stop();
  return ok;
}

// ===================== WIFI =====================
bool connecterWiFi() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(true);
  WiFi.begin(cfg_ssid.c_str(), cfg_pass.c_str());

  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - t0) < WIFI_TIMEOUT_MS) {
    delay(120);
  }
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.setTxPower(WIFI_PUISSANCE_TX);
    return true;
  }
  return false;
}

void synchroniserHeureSiBesoin() {
  uint32_t now = epochActuel();

  if (!heureValide() || (dernier_ntp_epoch == 0) || (now - dernier_ntp_epoch) > NTP_REFRESH_S) {
    configTzTime(TZ_INFO, NTP1, NTP2);

    uint32_t t0 = millis();
    while (!heureValide() && (millis() - t0) < 3000) delay(50);

    if (heureValide()) dernier_ntp_epoch = epochActuel();
  }
}

// ===================== CONFIG (NVS) =====================
bool chargerConfig() {
  prefs.begin("nichoir", true);
  cfg_ssid     = prefs.getString("ssid", "");
  cfg_pass     = prefs.getString("pass", "");
  cfg_rpi_host = prefs.getString("rpi",  "");
  cfg_rpi_port = prefs.getInt("port", RPI_PORT_DEFAUT);
  cfg_rpi_path = prefs.getString("path", RPI_PATH_DEFAUT);
  prefs.end();

  return (cfg_ssid.length() > 0) && (cfg_rpi_host.length() > 0);
}

// ===================== PORTAIL AP (liste Wi-Fi) =====================
static const char* PAGE_CONFIG = R"rawliteral(
<!doctype html><html lang="fr"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Configuration Nichoir</title>
<style>
  body{font-family:Arial;background:#f5f5f5;padding:16px}
  .box{max-width:520px;margin:auto;background:#fff;padding:16px;border-radius:14px;box-shadow:0 6px 20px rgba(0,0,0,.08)}
  h2{margin:0 0 10px 0}
  .hint{color:#666;font-size:13px;margin:6px 0 14px}
  .net{display:flex;align-items:center;justify-content:space-between;padding:10px 12px;border:1px solid #eee;border-radius:12px;margin:8px 0;cursor:pointer}
  .net:hover{background:#f7f9ff;border-color:#dbe7ff}
  .left{display:flex;flex-direction:column;gap:2px}
  .ssid{font-weight:700}
  .meta{color:#666;font-size:12px}
  .sig{font-family:monospace;color:#333}
  input{width:100%;padding:10px;margin:8px 0;border:1px solid #ddd;border-radius:10px}
  button{width:100%;padding:12px;border:0;border-radius:12px;background:#1967d2;color:#fff;font-size:16px;cursor:pointer}
  button:disabled{opacity:.5;cursor:not-allowed}
  .hr{height:1px;background:#eee;margin:12px 0}
  .row{display:flex;gap:10px}
  .row>div{flex:1}
  .small{font-size:12px;color:#777}
</style></head><body>
<div class="box">
  <h2>Configuration du Nichoir</h2>
  <div class="hint">Clique ton Wi-Fi (scan automatique), puis entre le mot de passe.</div>

  <div id="etat" class="small">Scan en cours…</div>
  <div id="liste"></div>

  <form method="POST" action="/save" onsubmit="return verifier();">
    <label class="small">SSID sélectionné</label>
    <input id="ssid" name="ssid" readonly placeholder="Clique un Wi-Fi dans la liste" required>

    <label class="small">Mot de passe (vide si réseau ouvert)</label>
    <input id="pass" name="pass" type="password" placeholder="••••••••">

    <div class="hr"></div>
    <h3 style="margin:0 0 6px 0">Raspberry Pi (réception)</h3>
    <label class="small">Hôte (IP ou nom)</label>
    <input name="rpi" placeholder="raspberrypi.local" required>

    <div class="row">
      <div><label class="small">Port</label><input name="port" value="5000" required></div>
      <div><label class="small">Chemin</label><input name="path" value="/upload" required></div>
    </div>

    <button id="btn" type="submit" disabled>Enregistrer & redémarrer</button>
  </form>
</div>

<script>
function barres(rssi){ if(rssi>-55) return "████"; if(rssi>-67) return "███░"; if(rssi>-80) return "██░░"; return "█░░░"; }
function securite(enc){ return enc ? "🔒 sécurisé" : "🔓 ouvert"; }
function choisir(ssid){ document.getElementById('ssid').value = ssid; document.getElementById('btn').disabled = false; }
function verifier(){ if(!document.getElementById('ssid').value.trim()){ alert("Choisis un Wi-Fi dans la liste."); return false; } return true; }

async function scan(){
  try{
    const r = await fetch('/scan', {cache:'no-store'});
    const d = await r.json();
    if(!d || !d.ok) return;

    document.getElementById('etat').textContent = d.n + " réseau(x) trouvé(s). Clique pour sélectionner.";
    const liste = document.getElementById('liste');
    liste.innerHTML = "";
    d.items.forEach(it=>{
      const div = document.createElement('div');
      div.className='net';
      div.onclick=()=>choisir(it.ssid);
      div.innerHTML = `<div class="left">
        <div class="ssid">${it.ssid || "(SSID masqué)"}</div>
        <div class="meta">${securite(it.enc)} • RSSI ${it.rssi} dBm</div>
      </div><div class="sig">${barres(it.rssi)}</div>`;
      liste.appendChild(div);
    });
  }catch(e){}
}

scan();
setInterval(scan, 2000);
</script>
</body></html>
)rawliteral";

void demarrerPortailConfig() {
  setCpuFrequencyMhz(80); // économie en mode portail

  String apSsid = "Nichoir-" + idAppareil();
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(apSsid.c_str(), AP_PASSWORD);
  IPAddress ip = WiFi.softAPIP();

  dnsServer.start(53, "*", ip);

  // scan asynchrone
  WiFi.scanNetworks(true, true);

  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", PAGE_CONFIG);
  });

  server.on("/scan", HTTP_GET, []() {
    int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_FAILED || n == -2) {
      WiFi.scanNetworks(true, true);
      server.send(200, "application/json", "{\"ok\":false}");
      return;
    }
    if (n == -1) {
      server.send(200, "application/json", "{\"ok\":false}");
      return;
    }

    String json = "{\"ok\":true,\"n\":" + String(n) + ",\"items\":[";
    for (int i = 0; i < n; i++) {
      String ssid = WiFi.SSID(i);
      ssid.replace("\\", "\\\\");
      ssid.replace("\"", "\\\"");
      int rssi = WiFi.RSSI(i);
      bool enc = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);

      json += "{\"ssid\":\"" + ssid + "\",\"rssi\":" + String(rssi) + ",\"enc\":" + String(enc ? "true" : "false") + "}";
      if (i < n - 1) json += ",";
    }
    json += "]}";

    WiFi.scanDelete();
    WiFi.scanNetworks(true, true);
    server.send(200, "application/json", json);
  });

  server.on("/save", HTTP_POST, []() {
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");
    String rpi  = server.arg("rpi");
    int port    = server.arg("port").toInt();
    String path = server.arg("path");

    prefs.begin("nichoir", false);
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    prefs.putString("rpi",  rpi);
    prefs.putInt("port", port);
    prefs.putString("path", path);
    prefs.end();

    server.send(200, "text/html", "<h3>OK. Redémarrage...</h3>");
    delay(600);
    ESP.restart();
  });

  server.onNotFound([ip]() {
    server.sendHeader("Location", String("http://") + ip.toString() + "/", true);
    server.send(302, "text/plain", "");
  });

  server.begin();

  Serial.println("=== MODE CONFIGURATION (AP) ===");
  Serial.print("SSID AP: "); Serial.println(apSsid);
  Serial.print("MDP AP : "); Serial.println(AP_PASSWORD);
  Serial.print("URL    : http://"); Serial.println(ip);

  // timeout autonomie
  uint32_t t0 = millis();
  while (millis() - t0 < AP_DUREE_MAX_MS) {
    dnsServer.processNextRequest();
    server.handleClient();
    delay(5);
  }

  // Personne n’a configuré -> deep sleep pour économiser
  Serial.println("Timeout portail -> deep-sleep (autonomie).");
  WiFi.mode(WIFI_OFF);
  btStop();
  esp_sleep_enable_timer_wakeup((uint64_t)SOMMEIL_AP_ECHEC_S * 1000000ULL);
  esp_deep_sleep_start();
}

// ===================== LOGIQUE RÉVEIL =====================
String causeReveil() {
  auto c = esp_sleep_get_wakeup_cause();
  if (c == ESP_SLEEP_WAKEUP_EXT0 || c == ESP_SLEEP_WAKEUP_EXT1) return "pir";
  if (c == ESP_SLEEP_WAKEUP_TIMER) return "timer";
  return "boot";
}

// Planifier le prochain sommeil (timer + PIR si autorisé)
void dormirAutonomie() {
  uint32_t now = epochActuel();

  // si pas d’heure (rare si Wi-Fi ok), fallback 24h
  if (!heureValide()) {
    esp_sleep_enable_timer_wakeup((uint64_t)INTERVALLE_JOUR_S * 1000000ULL);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_PIR, 1);
  } else {
    // gérer fin cooldown si dépassée
    if (cooldown_pir_actif && now >= cooldown_fin_epoch) {
      cooldown_pir_actif = false;
    }

    // calc prochaines échéances
    uint32_t sec_jour = (prochain_jour_epoch > now) ? (prochain_jour_epoch - now) : 5;
    uint32_t sec_cd   = 0xFFFFFFFF;
    if (cooldown_pir_actif) {
      sec_cd = (cooldown_fin_epoch > now) ? (cooldown_fin_epoch - now) : 5;
    }

    uint32_t prochain_timer = (sec_jour < sec_cd) ? sec_jour : sec_cd;
    if (prochain_timer < 5) prochain_timer = 5;

    esp_sleep_enable_timer_wakeup((uint64_t)prochain_timer * 1000000ULL);

    // PIR uniquement si pas en cooldown
    if (!cooldown_pir_actif) {
      esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_PIR, 1);
    }
  }

  // couper tout ce qui consomme
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  btStop();

  pinMode(PIN_LED_CARTE, OUTPUT);
  digitalWrite(PIN_LED_CARTE, LOW);
  gpio_hold_en((gpio_num_t)PIN_LED_CARTE);
  gpio_deep_sleep_hold_en();

  Serial.println("Deep-sleep...");
  Serial.flush();
  esp_deep_sleep_start();
}

// ===================== SETUP / LOOP =====================
void setup() {
  Serial.begin(115200);
  delay(150);

  pinMode(PIN_LED_CARTE, OUTPUT);
  digitalWrite(PIN_LED_CARTE, LOW);

  // Si pas de config -> portail AP
  if (!chargerConfig()) {
    demarrerPortailConfig();
    return; // ne doit jamais revenir
  }

  // Connexion Wi-Fi
  if (!connecterWiFi()) {
    // Wi-Fi invalide / plus dispo -> portail pour corriger
    demarrerPortailConfig();
    return;
  }

  // Heure (NTP rare)
  synchroniserHeureSiBesoin();

  uint32_t now = epochActuel();
  if (!heureValide()) {
    // Si vraiment pas d’heure, on fait simple: planifier 24h
    prochain_jour_epoch = 0;
  } else {
    if (prochain_jour_epoch == 0) prochain_jour_epoch = now + INTERVALLE_JOUR_S;
    if (cooldown_pir_actif && now >= cooldown_fin_epoch) cooldown_pir_actif = false;
  }

  String cause = causeReveil();
  bool fairePhoto = false;
  String raison = "";

  // 1) Photo quotidienne
  if (heureValide() && now >= prochain_jour_epoch) {
    fairePhoto = true;
    raison = "jour";
    prochain_jour_epoch = now + INTERVALLE_JOUR_S;
  }

  // 2) Photo PIR (si pas en cooldown)
  if (cause == "pir" && !cooldown_pir_actif) {
    fairePhoto = true;
    if (raison.length() == 0) raison = "pir";
    else raison = "jour_pir"; // si les deux tombent ensemble

    cooldown_pir_actif = true;
    cooldown_fin_epoch = (heureValide() ? (now + COOLDOWN_PIR_S) : 0);
  }

  // 3) Premier boot: on ne force pas de photo (autonomie)
  // (si tu veux une photo au premier boot, décommente:)
  // if (cause == "boot" && !fairePhoto) { fairePhoto = true; raison = "boot"; }

  if (!fairePhoto) {
    dormirAutonomie();
    return;
  }

  // Choix taille/qualité (autonomie + transfert)
  // PIR = plus léger (QVGA), JOUR = un peu mieux (VGA)
  framesize_t taille = (raison.indexOf("pir") >= 0) ? FRAMESIZE_QVGA : FRAMESIZE_VGA;
  int qualite = (raison.indexOf("pir") >= 0) ? 14 : 12; // plus grand = plus compressé

  if (!initialiserCamera(taille, qualite)) {
    Serial.println("Erreur init caméra.");
    dormirAutonomie();
    return;
  }

  float vbatt = lireBatterieVoltsApprox();
  bool flash = flashDoitEtreActive(vbatt);

  camera_fb_t* fb = capturerImage(flash);
  bool ok = false;
  if (fb) {
    ok = envoyerAuRpi(fb, raison, vbatt, flash);
    esp_camera_fb_return(fb);
  }

  esp_camera_deinit();

  Serial.println(ok ? "Envoi OK" : "Envoi ÉCHEC");

  // Dodo immédiat
  dormirAutonomie();
}

void loop() {
  // Inutilisé: on dort tout le temps sauf portail AP (géré dans demarrerPortailConfig())
}
