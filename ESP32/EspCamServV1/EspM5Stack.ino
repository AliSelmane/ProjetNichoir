#include <WiFi.h>
#include <PubSubClient.h>
#include "esp_camera.h"


#define SLEEP_TIME 10  // Time in seconds before sleep
#define CAMERA_MODEL_M5STACK_PSRAM
#include "camera_pins.h"
#include "camera_index.h"

// MQTT Configuration
const char* mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;
const char* topic_pub = "timerCam/test";
const char* topic_sub = "timerCam/cmd";

// WiFi credentials
const char* ssid = "iPhone de Ali";
const char* password = "crevard123";

// MQTT Client
WiFiClient espClient;
PubSubClient Client(espClient);

void setup() {
  Serial.begin(115200);  // Initialisation correcte du port série
  Serial.setDebugOutput(true);
  Serial.println();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG;  // for streaming
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  if (psramFound()) {
    config.jpeg_quality = 10;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");

  Client.setServer(mqtt_server, mqtt_port);
  Client.setCallback(mqtt_callback);

  /*startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");*/
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("[MQTT] Message reçu sur ");
    Serial.print(topic);
    Serial.print(" : ");
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();
}

void reconnect_mqtt() {
    while (!Client.connected()) {
        Serial.print("[MQTT] Connexion au broker… ");
        if (Client.connect("TimerCamClient")) {
            Serial.println("OK !");
            Client.subscribe(topic_sub);
            Client.publish(topic_pub, "TimerCam connectée !");
        } else {
            Serial.print("ÉCHEC, code = ");
            Serial.println(Client.state());
            delay(3000);
        }
    }
}

// Fonction pour capturer l'image
camera_fb_t* capture() {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
        return nullptr;
    }
    return fb;
}

// Fonction pour envoyer l'image via MQTT
void send(camera_fb_t* fb) {
    if (fb == nullptr) {
        Serial.println("No frame to send");
        return;
    }

    // Afficher la taille de l'image capturée avant de l'envoyer
    Serial.print("Image size: ");
    Serial.print(fb->len);  // Affiche la taille de l'image en octets
    Serial.println(" bytes");

    // Publish image as JPEG via MQTT
    char topic[128];

    Serial.print(topic[0]);
    snprintf(topic, sizeof(topic), "%s", topic_pub);

    Client.publish(topic, fb->buf, fb->len);
    Serial.println("[MQTT] Image capturée et envoyée");

    esp_camera_fb_return(fb);  // Libère la mémoire utilisée pour le frame
}

void loop() {
    if (!Client.connected()) {
        reconnect_mqtt();
    }

    Client.loop();

    static unsigned long lastCaptureTime = 0;
    unsigned long currentMillis = millis();

    // Capture image between 60 to 300 seconds interval
    if (currentMillis - lastCaptureTime > 60000 && currentMillis - lastCaptureTime <= 300000) {
        lastCaptureTime = currentMillis;
        
        // Capture and send the image
        camera_fb_t* fb = capture();  // Capture image
        send(fb);  // Send image via MQTT
    }

    delay(100);  // Add delay to avoid unnecessary CPU usage
}
