#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

// ====== CONFIG WIFI/MQTT ======
const char* WIFI_SSID   = "IoT-Demo-Lab";
const char* WIFI_PASS   = "Tutu2929";
const char* MQTT_BROKER = "192.168.10.1";    // IP de tu Raspberry Pi
const uint16_t MQTT_PORT = 1883;
const char* MQTT_USER   = "esp32user";       // si habilitas auth en Mosquitto
const char* MQTT_PASS   = "esp32pass";       // idem
const char* BASE_TOPIC  = "home/lab/esp32s3"; // prefijo de tus topics

// Topic para comandos de alarma (apagado, enable/disable, etc.)
const char* ALARM_CMD_TOPIC = "home/lab/esp32s3/alarm/cmd";

// ====== PINES ======
#define DHTPIN      4
#define DHTTYPE     DHT11
#define PIR_PIN     5

// Pin para la mini bocina (buzzer activo de preferencia)
#define BUZZER_PIN  6    // usa un GPIO libre de la ESP32-S3

// Publicación cada X ms
const unsigned long PUB_MS = 5000;

// Umbral de MOTION seguidos para disparar la alarma
const int MOTION_STREAK_THRESHOLD = 10;
// Duración de la alarma en milisegundos
const unsigned long ALARM_DURATION_MS = 5000;  // 5 s

DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient mqtt(espClient);
unsigned long lastPub = 0;
int lastPir = -1;

// Variables para conteo de MOTION y alarma
int motionStreak = 0;
bool alarmActive = false;      // ¿La alarma está sonando?
bool alarmEnabled = true;      // ¿La lógica de alarma está armada?
unsigned long alarmEndTime = 0;
unsigned long lastBeepToggle = 0;

// Prototipo del callback MQTT
void mqttCallback(char* topic, byte* payload, unsigned int length);

String clientId() {
  uint8_t mac[6]; 
  WiFi.macAddress(mac);
  char buf[32];
  snprintf(buf, sizeof(buf), "esp32s3-%02X%02X%02X", mac[3], mac[4], mac[5]);
  return String(buf);
}

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Conectando WiFi");
  while (WiFi.status() != WL_CONNECTED) { 
    Serial.print("."); 
    delay(500); 
  }
  Serial.print("\nWiFi OK. IP: "); 
  Serial.println(WiFi.localIP());
}

void publishAlarmEnabledState() {
  char topic[96];
  snprintf(topic, sizeof(topic), "%s/alarm/enabled", BASE_TOPIC);
  mqtt.publish(topic, alarmEnabled ? "true" : "false", true);
}

void publishAlarmActiveState() {
  char topic[96];
  snprintf(topic, sizeof(topic), "%s/alarm/state", BASE_TOPIC);
  mqtt.publish(topic, alarmActive ? "ON" : "OFF", true);
}

void connectMQTT() {
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);   // Registrar callback

  String cid = clientId();
  String willTopic = String(BASE_TOPIC) + "/lwt";
  const char* willMsg = "OFFLINE";

  while (!mqtt.connected()) {
    Serial.print("Conectando MQTT...");
    bool ok;
    if (MQTT_USER && strlen(MQTT_USER) > 0) {
      ok = mqtt.connect(
        cid.c_str(),
        MQTT_USER,
        MQTT_PASS,
        willTopic.c_str(),
        1,
        true,
        willMsg
      );
    } else {
      ok = mqtt.connect(
        cid.c_str(),
        nullptr,
        nullptr,
        willTopic.c_str(),
        1,
        true,
        willMsg
      );
    }

    if (ok) {
      Serial.println("OK");
      mqtt.publish(willTopic.c_str(), "ONLINE", true);

      // Suscribirse al topic de comandos de alarma
      if (mqtt.subscribe(ALARM_CMD_TOPIC)) {
        Serial.print("Suscrito a: ");
        Serial.println(ALARM_CMD_TOPIC);
      } else {
        Serial.println("Error al suscribirse a ALARM_CMD_TOPIC");
      }

      // Publicar estados iniciales
      publishAlarmEnabledState();
      publishAlarmActiveState();

    } else {
      Serial.print(" fail rc="); 
      Serial.println(mqtt.state());
      delay(1000);
    }
  }
}

// Callback para mensajes MQTT entrantes
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Convertir payload a String
  String msg;
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  Serial.print("MQTT msg en [");
  Serial.print(topic);
  Serial.print("] = ");
  Serial.println(msg);

  // Manejo de comandos de alarma
  if (String(topic) == String(ALARM_CMD_TOPIC)) {
    msg.toUpperCase();

    // Silenciar la alarma actual, pero sin deshabilitarla permanentemente
    if (msg == "OFF" || msg == "STOP" || msg == "SILENCE") {
      alarmActive = false;
      motionStreak = 0;
      digitalWrite(BUZZER_PIN, LOW);
      Serial.println(">>> Comando: SILENCIAR ALARMA / BUZZER OFF <<<");
      publishAlarmActiveState();
    }

    // Deshabilitar completamente la lógica de alarma (no se dispara hasta ENABLE)
    if (msg == "DISABLE" || msg == "DISARM") {
      alarmEnabled = false;
      alarmActive = false;
      motionStreak = 0;
      digitalWrite(BUZZER_PIN, LOW);
      Serial.println(">>> Comando: ALARMA DESARMADA (DISABLE) <<<");
      publishAlarmEnabledState();
      publishAlarmActiveState();
    }

    // Volver a habilitar la lógica de alarma
    if (msg == "ENABLE" || msg == "ARM") {
      alarmEnabled = true;
      motionStreak = 0;
      Serial.println(">>> Comando: ALARMA ARMADA (ENABLE) <<<");
      publishAlarmEnabledState();
      // alarmActive se mantiene como esté (normalmente OFF)
    }

    // Aquí podrías agregar otros comandos como "TEST", etc.
  }
}

// Manejar la bocina (alarma) sin bloquear el loop
void handleAlarm() {
  // Si la alarma no está activa o está deshabilitada, buzzer siempre OFF
  if (!alarmActive || !alarmEnabled) {
    digitalWrite(BUZZER_PIN, LOW);
    return;
  }

  unsigned long now = millis();

  // Si ya se acabó el tiempo de alarma, apagamos
  if (now >= alarmEndTime) {
    alarmActive = false;
    motionStreak = 0;
    digitalWrite(BUZZER_PIN, LOW);
    Serial.println("Alarma finalizada, buzzer OFF");
    publishAlarmActiveState();
    return;
  }

  // Pequeño patrón "beep" alternando estado cada 200 ms
  if (now - lastBeepToggle >= 200) {
    lastBeepToggle = now;
    int current = digitalRead(BUZZER_PIN);
    digitalWrite(BUZZER_PIN, !current);
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(PIR_PIN, INPUT_PULLDOWN);  // IMPORTANTE en ESP32
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);     // Buzzer apagado al inicio

  dht.begin();

  Serial.println("Iniciando ESP32S3 + DHT11 + PIR + MQTT + BUZZER...");
  connectWiFi();
  connectMQTT();
}

void loop() {
  // Mantener WiFi/MQTT
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqtt.connected()) connectMQTT();
  mqtt.loop();

  // Leer PIR
  int pir = digitalRead(PIR_PIN);

  // === LÓGICA DE CONTEO DE MOTION SEGUIDOS ===
  if (alarmEnabled) {
    if (pir == HIGH) {
      if (motionStreak < MOTION_STREAK_THRESHOLD) {
        motionStreak++;
      }
    } else {
      // Si el sensor se pone en CLEAR, reiniciamos el conteo
      motionStreak = 0;
    }
  } else {
    // Si la alarma está deshabilitada, no contamos
    motionStreak = 0;
  }

  // Si aún no está activa la alarma y se alcanzó el umbral, y además está habilitada
  if (!alarmActive && alarmEnabled && motionStreak >= MOTION_STREAK_THRESHOLD) {
    alarmActive = true;
    alarmEndTime = millis() + ALARM_DURATION_MS;
    lastBeepToggle = millis();     // para iniciar el patrón de beep
    Serial.println(">>> UMBRAL DE MOTION ALCANZADO: ALARMA ACTIVADA <<<");
    publishAlarmActiveState();
  }

  // Debug periódico de PIR
  static unsigned long lastDebug = 0;
  if (millis() - lastDebug > 2000) {
    lastDebug = millis();
    Serial.print("PIR raw: ");
    Serial.print(pir);
    Serial.print(" -> ");
    Serial.print(pir ? "MOTION" : "CLEAR");
    Serial.print(" | motionStreak = ");
    Serial.print(motionStreak);
    Serial.print(" | alarmEnabled = ");
    Serial.print(alarmEnabled ? "true" : "false");
    Serial.print(" | alarmActive = ");
    Serial.println(alarmActive ? "true" : "false");
  }

  // Publica cambio de estado del PIR
  if (pir != lastPir) {
    lastPir = pir;

    char topic[96], payload[32];
    snprintf(topic, sizeof(topic), "%s/pir/state", BASE_TOPIC);
    snprintf(payload, sizeof(payload), "%s", pir ? "MOTION" : "CLEAR");

    mqtt.publish(topic, payload, true);

    Serial.print("PIR cambio a: ");
    Serial.println(pir ? "MOTION" : "CLEAR");
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
      // Temperatura
      snprintf(topic, sizeof(topic), "%s/dht/temp", BASE_TOPIC);
      dtostrf(t, 0, 2, payload);
      mqtt.publish(topic, payload);

      // Humedad
      snprintf(topic, sizeof(topic), "%s/dht/hum", BASE_TOPIC);
      dtostrf(h, 0, 2, payload);
      mqtt.publish(topic, payload);
    }

    // JSON agregado con t, h, pir, streak y estados de alarma
    snprintf(topic, sizeof(topic), "%s/tele", BASE_TOPIC);
    snprintf(payload, sizeof(payload),
      "{\"t\":%.2f,\"h\":%.2f,\"pir\":\"%s\",\"streak\":%d,\"alarm\":%s,\"alarmEnabled\":%s}",
      ok ? t : -127.0f,
      ok ? h : -1.0f,
      pir ? "MOTION" : "CLEAR",
      motionStreak,
      alarmActive ? "true" : "false",
      alarmEnabled ? "true" : "false"
    );

    mqtt.publish(topic, payload);
    Serial.print("TELE -> ");
    Serial.println(payload);
  }

  // Manejar la bocina (alarma) sin bloquear el loop
  handleAlarm();
}
