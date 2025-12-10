#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

// ====== CONFIG WIFI/MQTT ======
const char* WIFI_SSID   = "IoT-Demo-Lab";
const char* WIFI_PASS   = "Tutu2929";
const char* MQTT_BROKER = "192.168.10.1";    // IP del broker (Raspberry / servidor)
const uint16_t MQTT_PORT = 1883;
const char* MQTT_USER   = "esp32user";
const char* MQTT_PASS   = "esp32pass";

// Topic base y de comando del servo
const char* BASE_TOPIC      = "home/lab/esp32s3/servo";
const char* SERVO_CMD_TOPIC = "home/lab/esp32s3/servo/cmd";    // aquí mandas abrir_garaje / cerrar_garaje
const char* SERVO_STATE_TOPIC = "home/lab/esp32s3/servo/state"; // publica CERRADO/ABIERTO

// ====== SERVO ======
const int PIN_SERVO        = 17;
const int POSICION_INICIO  = 0;    // 0 grados
const int POSICION_AVANCE  = 90;   // 90 grados

Servo miServo;

// Variable de estado:
// 0: Servo en la posición de INICIO (0 grados)  -> "CERRADO"
// 1: Servo en la posición de AVANCE (90 grados) -> "ABIERTO"
int estadoActual = 0;

// WiFi / MQTT
WiFiClient espClient;
PubSubClient mqtt(espClient);

// ====== FUNCIONES AUXILIARES ======
String clientId() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char buf[32];
  snprintf(buf, sizeof(buf), "esp32s3-servo-%02X%02X%02X", mac[3], mac[4], mac[5]);
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

void publishServoState() {
  if (!mqtt.connected()) return;

  const char* estadoStr = (estadoActual == 0) ? "CERRADO" : "ABIERTO";
  mqtt.publish(SERVO_STATE_TOPIC, estadoStr, true);

  Serial.print("Estado servo publicado: ");
  Serial.println(estadoStr);
}

void connectMQTT() {
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);

  // Registramos el callback antes de conectar
  extern void mqttCallback(char*, byte*, unsigned int);
  mqtt.setCallback(mqttCallback);

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

      // Suscribir al topic de comando del servo
      if (mqtt.subscribe(SERVO_CMD_TOPIC)) {
        Serial.print("Suscrito a: ");
        Serial.println(SERVO_CMD_TOPIC);
      } else {
        Serial.println("Error al suscribirse a SERVO_CMD_TOPIC");
      }

      // Publicar estado inicial del servo
      publishServoState();

    } else {
      Serial.print(" fail rc=");
      Serial.println(mqtt.state());
      delay(1000);
    }
  }
}

// ====== LÓGICA DEL SERVO (ABRIR/CERRAR) ======
void abrirGaraje() {
  if (estadoActual == 0) { // solo si está CERRADO (0°)
    Serial.println("Comando MQTT: abrir_garaje");
    miServo.write(POSICION_AVANCE);
    delay(800); // pequeño tiempo para que llegue a la posición (ajusta si hace falta)
    estadoActual = 1;
    Serial.println("Servo AVANZÓ a 90 grados (ABIERTO).");
    publishServoState();
  } else {
    Serial.println("Ya está en 90 grados (ABIERTO). Ignorando abrir_garaje.");
  }
}

void cerrarGaraje() {
  if (estadoActual == 1) { // solo si está ABIERTO (90°)
    Serial.println("Comando MQTT: cerrar_garaje");
    miServo.write(POSICION_INICIO);
    delay(800);
    estadoActual = 0;
    Serial.println("Servo RETROCEDIÓ a 0 grados (CERRADO).");
    publishServoState();
  } else {
    Serial.println("Ya está en 0 grados (CERRADO). Ignorando cerrar_garaje.");
  }
}

// ====== CALLBACK MQTT ======
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  Serial.print("MQTT msg en [");
  Serial.print(topic);
  Serial.print("] = ");
  Serial.println(msg);

  String t = String(topic);

  if (t == SERVO_CMD_TOPIC) {
    msg.trim();
    msg.toLowerCase();

    if (msg == "abrir_garaje") {
      abrirGaraje();
    } else if (msg == "cerrar_garaje") {
      cerrarGaraje();
    } else {
      Serial.print("Comando no reconocido en SERVO_CMD_TOPIC: ");
      Serial.println(msg);
    }
  }
}

// ====== SETUP / LOOP ======
void setup() {
  Serial.begin(115200);
  Serial.println("--- Control de Servo por MQTT ---");
  Serial.println("Comandos por MQTT en topic:");
  Serial.println("  home/lab/esp32s3/servo/cmd");
  Serial.println("Payloads válidos: abrir_garaje, cerrar_garaje");
  Serial.println("Restricción: solo alterna entre 0° y 90° (no dobles avances/retrocesos).");

  // Configurar servo
  miServo.attach(PIN_SERVO, 500, 2500); // min_us y max_us según tu servo
  miServo.write(POSICION_INICIO);
  delay(1000);
  estadoActual = 0;

  connectWiFi();
  connectMQTT();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqtt.connected()) connectMQTT();
  mqtt.loop();

  // (Opcional) seguir aceptando comandos por Serial para debug
  if (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '1') {
      abrirGaraje();
    } else if (c == '0') {
      cerrarGaraje();
    }
  }
}
