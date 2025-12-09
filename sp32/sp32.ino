#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

// ====== CONFIG WIFI/MQTT ======
const char* WIFI_SSID   = "TU_SSID";
const char* WIFI_PASS   = "TU_PASSWORD";
const char* MQTT_BROKER = "192.168.1.50";    // IP de tu Raspberry Pi
const uint16_t MQTT_PORT = 1883;
const char* MQTT_USER   = "esp32user";       // si habilitas auth en Mosquitto
const char* MQTT_PASS   = "esp32pass";       // idem
const char* BASE_TOPIC  = "home/lab/esp32s3"; // prefijo de tus topics

// ====== PINES ======
#define DHTPIN   4
#define DHTTYPE  DHT11
#define PIR_PIN  5

// Publicación cada X ms
const unsigned long PUB_MS = 5000;

DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient mqtt(espClient);
unsigned long lastPub = 0;
int lastPir = -1;

String clientId() {
  uint8_t mac[6]; WiFi.macAddress(mac);
  char buf[32];
  snprintf(buf, sizeof(buf), "esp32s3-%02X%02X%02X", mac[3], mac[4], mac[5]);
  return String(buf);
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Conectando WiFi");
  while (WiFi.status() != WL_CONNECTED) { Serial.print("."); delay(500); }
  Serial.print("\nWiFi OK. IP: "); Serial.println(WiFi.localIP());
}

void connectMQTT() {
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  String cid = clientId();
  // Last Will: estado OFFLINE si se cae
  String willTopic = String(BASE_TOPIC) + "/lwt";
  const char* willMsg = "OFFLINE";
  while (!mqtt.connected()) {
    Serial.print("Conectando MQTT...");
    bool ok;
    if (MQTT_USER && strlen(MQTT_USER)>0) {
      ok = mqtt.connect(cid.c_str(), MQTT_USER, MQTT_PASS,
                        willTopic.c_str(), 1, true, willMsg);
    } else {
      ok = mqtt.connect(cid.c_str(), nullptr, nullptr,
                        willTopic.c_str(), 1, true, willMsg);
    }
    if (ok) {
      Serial.println("OK");
      mqtt.publish(willTopic.c_str(), "ONLINE", true);
    } else {
      Serial.print(" fail rc="); Serial.println(mqtt.state());
      delay(1000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIR_PIN, INPUT);     // El HC-SR501 ya tiene pull-ups/downs internos
  dht.begin();
  delay(100);
  connectWiFi();
  connectMQTT();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqtt.connected()) connectMQTT();
  mqtt.loop();

  // Publica cuando cambie el PIR
  int pir = digitalRead(PIR_PIN);
  if (pir != lastPir) {
    lastPir = pir;
    char topic[96], payload[32];
    snprintf(topic, sizeof(topic), "%s/pir/state", BASE_TOPIC);
    snprintf(payload, sizeof(payload), "%s", pir ? "MOTION" : "CLEAR");
    mqtt.publish(topic, payload, true);
  }

  // Publicación periódica (DHT + estado agregado)
  unsigned long now = millis();
  if (now - lastPub >= PUB_MS) {
    lastPub = now;

    float h = dht.readHumidity();
    float t = dht.readTemperature(); // °C
    bool ok = !(isnan(h) || isnan(t));

    char topic[96], payload[128];
    if (ok) {
      snprintf(topic, sizeof(topic), "%s/dht/temp", BASE_TOPIC);
      dtostrf(t, 0, 2, payload); mqtt.publish(topic, payload);

      snprintf(topic, sizeof(topic), "%s/dht/hum", BASE_TOPIC);
      dtostrf(h, 0, 2, payload); mqtt.publish(topic, payload);
    }

    // JSON agregado
    snprintf(topic, sizeof(topic), "%s/tele", BASE_TOPIC);
    snprintf(payload, sizeof(payload),
      "{\"t\":%.2f,\"h\":%.2f,\"pir\":\"%s\"}",
      ok ? t : -127.0f, ok ? h : -1.0f, pir ? "MOTION" : "CLEAR");
    mqtt.publish(topic, payload);

    Serial.println(payload);
  }
}
