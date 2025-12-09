#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>

// ====== CONFIG WIFI/MQTT ======
const char* WIFI_SSID   = "IoT-Demo-Lab";
const char* WIFI_PASS   = "Tutu2929";
const char* MQTT_BROKER = "192.168.10.1";  // IP de tu Raspberry Pi
const uint16_t MQTT_PORT = 1883;
const char* MQTT_USER   = "esp32user";
const char* MQTT_PASS   = "esp32pass";

// Topics de comando para cada tira
const char* STRIP1_CMD_TOPIC = "home/lab/esp32s3/led/strip1/cmd";
const char* STRIP2_CMD_TOPIC = "home/lab/esp32s3/led/strip2/cmd";
const char* STRIP3_CMD_TOPIC = "home/lab/esp32s3/led/strip3/cmd";

// ====== CONFIG NEOPIXEL ======
const int STRIP1_PIN = 38;
const int STRIP2_PIN = 2;
const int STRIP3_PIN = 3;
const int LED_COUNT  = 8;  // ajusta según tus tiras reales

Adafruit_NeoPixel strip1(LED_COUNT, STRIP1_PIN, NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel strip2(LED_COUNT, STRIP2_PIN, NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel strip3(LED_COUNT, STRIP3_PIN, NEO_RGB + NEO_KHZ800);

// ====== WIFI / MQTT ======
WiFiClient espClient;
PubSubClient mqtt(espClient);

// Prototipos
void connectWiFi();
void connectMQTT();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void handleStripCommand(Adafruit_NeoPixel &strip, const String &msg);

// ====== FUNCIONES AUXILIARES ======
String clientId() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char buf[32];
  snprintf(buf, sizeof(buf), "esp32s3-led-%02X%02X%02X", mac[3], mac[4], mac[5]);
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

void connectMQTT() {
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);

  String cid = clientId();

  while (!mqtt.connected()) {
    Serial.print("Conectando MQTT...");
    bool ok;
    if (MQTT_USER && strlen(MQTT_USER) > 0) {
      ok = mqtt.connect(
        cid.c_str(),
        MQTT_USER,
        MQTT_PASS
      );
    } else {
      ok = mqtt.connect(cid.c_str());
    }

    if (ok) {
      Serial.println("OK");

      // Suscribirse a los topics de las 3 tiras
      if (mqtt.subscribe(STRIP1_CMD_TOPIC))
        Serial.println(String("Suscrito a: ") + STRIP1_CMD_TOPIC);
      if (mqtt.subscribe(STRIP2_CMD_TOPIC))
        Serial.println(String("Suscrito a: ") + STRIP2_CMD_TOPIC);
      if (mqtt.subscribe(STRIP3_CMD_TOPIC))
        Serial.println(String("Suscrito a: ") + STRIP3_CMD_TOPIC);

    } else {
      Serial.print(" fail rc=");
      Serial.println(mqtt.state());
      delay(1000);
    }
  }
}

// Aplica un comando a una tira:
// - "OFF" -> apaga la tira
// - "R,G,B" -> color sólido para todos los LEDs
void handleStripCommand(Adafruit_NeoPixel &strip, const String &msg) {
  String cmd = msg;
  cmd.trim();

  Serial.print("Comando recibido para strip: ");
  Serial.println(cmd);

  // Apagar tira
  if (cmd.equalsIgnoreCase("OFF")) {
    strip.clear();
    strip.show();
    return;
  }

  // Opcional: algunos nombres de colores rápidos
  if (cmd.equalsIgnoreCase("RED")) {
    uint32_t c = strip.Color(255, 0, 0);
    for (int i = 0; i < strip.numPixels(); i++) strip.setPixelColor(i, c);
    strip.show();
    return;
  }
  if (cmd.equalsIgnoreCase("GREEN")) {
    uint32_t c = strip.Color(0, 255, 0);
    for (int i = 0; i < strip.numPixels(); i++) strip.setPixelColor(i, c);
    strip.show();
    return;
  }
  if (cmd.equalsIgnoreCase("BLUE")) {
    uint32_t c = strip.Color(0, 0, 255);
    for (int i = 0; i < strip.numPixels(); i++) strip.setPixelColor(i, c);
    strip.show();
    return;
  }
  if (cmd.equalsIgnoreCase("WHITE")) {
    uint32_t c = strip.Color(255, 255, 255);
    for (int i = 0; i < strip.numPixels(); i++) strip.setPixelColor(i, c);
    strip.show();
    return;
  }

  // Intentar parsear "R,G,B"
  int r, g, b;
  if (sscanf(cmd.c_str(), "%d,%d,%d", &r, &g, &b) == 3) {
    // limitar valores a 0-255
    r = constrain(r, 0, 255);
    g = constrain(g, 0, 255);
    b = constrain(b, 0, 255);

    uint32_t c = strip.Color(r, g, b);
    for (int i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
    }
    strip.show();

    Serial.print("Color aplicado: ");
    Serial.print(r); Serial.print(",");
    Serial.print(g); Serial.print(",");
    Serial.println(b);
  } else {
    Serial.println("Formato no reconocido. Usa OFF, RED, GREEN, BLUE, WHITE o R,G,B");
  }
}

// Callback MQTT: decide a qué tira aplicar el comando
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  Serial.print("MQTT msg en [");
  Serial.print(topic);
  Serial.print("] = ");
  Serial.println(msg);

  if (String(topic) == STRIP1_CMD_TOPIC) {
    handleStripCommand(strip1, msg);
  } else if (String(topic) == STRIP2_CMD_TOPIC) {
    handleStripCommand(strip2, msg);
  } else if (String(topic) == STRIP3_CMD_TOPIC) {
    handleStripCommand(strip3, msg);
  }
}

// ====== SETUP / LOOP ======
void setup() {
  Serial.begin(115200);
  delay(500);

  // Inicializar strips
  strip1.begin();
  strip2.begin();
  strip3.begin();

  strip1.clear();
  strip2.clear();
  strip3.clear();

  strip1.show();
  strip2.show();
  strip3.show();

  Serial.println("Iniciando ESP32S3 + 3 tiras NeoPixel + MQTT...");
  connectWiFi();
  connectMQTT();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqtt.connected()) connectMQTT();
  mqtt.loop();

  // Aquí podrías agregar algún efecto por defecto si quieres,
  // pero en esta versión las tiras solo cambian cuando llega un comando MQTT.
}
