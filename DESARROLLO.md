# 🚀 Plan de Desarrollo - Módulo Genérico IoT Domótico

## 📋 Estrategia Recomendada: Enfoque Incremental

### Filosofía: **Crawl → Walk → Run**

1. **Crawl (Rastreo)**: Infraestructura mínima viable + 1 dispositivo
2. **Walk (Caminar)**: Orquestación básica + múltiples dispositivos
3. **Run (Correr)**: Sistema completo con automatización avanzada

---

## 🎯 FASE 1: Infraestructura Base (Semana 1-2)

### Objetivo
Tener la Raspberry Pi 5 funcionando con servicios Docker básicos y comunicación MQTT funcional.

### Checklist de Tareas

#### 1.1 Preparación de Hardware
```bash
Hardware necesario (Costo aprox: $150-200 USD):
- [x] Raspberry Pi 5 (4GB o 8GB)           ~$80-100
- [x] MicroSD Card (64GB, Clase 10/A2)     ~$15
- [x] Fuente de alimentación oficial       ~$12
- [x] Case con ventilación                 ~$10
- [x] Cable Ethernet Cat6                  ~$5
- [ ] SSD USB 3.0 (128GB - opcional)       ~$25
```

#### 1.2 Instalación del Sistema Operativo
```bash
# Opción 1: Raspberry Pi OS (64-bit) - Recomendada para principiantes
$ sudo apt update && sudo apt upgrade -y
$ uname -a  # Verificar kernel 64-bit

# Opción 2: Ubuntu Server 22.04 LTS - Mejor para producción
# Descargar desde: https://ubuntu.com/download/raspberry-pi
```

#### 1.3 Configuración de Red
```bash
# Configurar IP estática
$ sudo nano /etc/dhcpcd.conf

# Agregar:
interface eth0
static ip_address=192.168.1.10/24
static routers=192.168.1.1
static domain_name_servers=8.8.8.8 8.8.4.4

# Reiniciar servicio de red
$ sudo systemctl restart dhcpcd

# Verificar
$ ip addr show eth0
```

#### 1.4 Instalación de Docker
```bash
# Script de instalación oficial
$ curl -fsSL https://get.docker.com -o get-docker.sh
$ sudo sh get-docker.sh

# Agregar usuario al grupo docker
$ sudo usermod -aG docker $USER
$ newgrp docker

# Instalar Docker Compose
$ sudo apt install docker-compose -y

# Verificar instalación
$ docker --version
$ docker-compose --version

# Habilitar Docker al inicio
$ sudo systemctl enable docker
```

#### 1.5 Crear Estructura de Proyecto
```bash
# Crear directorios
$ mkdir -p ~/iot-project/{config,data,logs,backups}
$ cd ~/iot-project

# Estructura recomendada:
iot-project/
├── docker-compose.yml          # Orquestación de servicios
├── .env                        # Variables de entorno (NO subir a Git)
├── config/
│   ├── mosquitto/
│   │   ├── mosquitto.conf
│   │   ├── passwd
│   │   └── acl
│   ├── nodered/
│   │   └── settings.js
│   ├── influxdb/
│   │   └── influxdb.conf
│   └── grafana/
│       └── provisioning/
├── data/                       # Volúmenes persistentes
├── logs/                       # Logs de servicios
└── backups/                    # Backups automáticos
```

#### 1.6 Crear docker-compose.yml Mínimo
```yaml
# Archivo: ~/iot-project/docker-compose.yml
version: '3.8'

services:
  # MQTT Broker
  mosquitto:
    image: eclipse-mosquitto:2.0
    container_name: mosquitto
    restart: unless-stopped
    ports:
      - "1883:1883"
      - "9001:9001"
    volumes:
      - ./config/mosquitto:/mosquitto/config
      - mosquitto_data:/mosquitto/data
      - mosquitto_log:/mosquitto/log
    networks:
      - iot_net

  # Node-RED (Orquestación)
  nodered:
    image: nodered/node-red:latest
    container_name: nodered
    restart: unless-stopped
    ports:
      - "1880:1880"
    volumes:
      - nodered_data:/data
    environment:
      - TZ=America/Bogota
    depends_on:
      - mosquitto
    networks:
      - iot_net

  # InfluxDB (Time-Series Database)
  influxdb:
    image: influxdb:2.7
    container_name: influxdb
    restart: unless-stopped
    ports:
      - "8086:8086"
    volumes:
      - influxdb_data:/var/lib/influxdb2
      - influxdb_config:/etc/influxdb2
    environment:
      - DOCKER_INFLUXDB_INIT_MODE=setup
      - DOCKER_INFLUXDB_INIT_USERNAME=admin
      - DOCKER_INFLUXDB_INIT_PASSWORD=adminpassword123
      - DOCKER_INFLUXDB_INIT_ORG=smarthome
      - DOCKER_INFLUXDB_INIT_BUCKET=iot_data
      - DOCKER_INFLUXDB_INIT_RETENTION=168h
      - DOCKER_INFLUXDB_INIT_ADMIN_TOKEN=mi-token-super-secreto-12345
    networks:
      - iot_net

  # Grafana (Dashboards)
  grafana:
    image: grafana/grafana:latest
    container_name: grafana
    restart: unless-stopped
    ports:
      - "3000:3000"
    volumes:
      - grafana_data:/var/lib/grafana
      - ./config/grafana/provisioning:/etc/grafana/provisioning
    environment:
      - GF_SECURITY_ADMIN_USER=admin
      - GF_SECURITY_ADMIN_PASSWORD=admin123
      - GF_INSTALL_PLUGINS=
      - GF_SERVER_ROOT_URL=http://192.168.1.10:3000
    depends_on:
      - influxdb
    networks:
      - iot_net

networks:
  iot_net:
    driver: bridge

volumes:
  mosquitto_data:
  mosquitto_log:
  nodered_data:
  influxdb_data:
  influxdb_config:
  grafana_data:
```

#### 1.7 Configurar Mosquitto
```bash
# Crear archivo de configuración
$ mkdir -p ~/iot-project/config/mosquitto
$ nano ~/iot-project/config/mosquitto/mosquitto.conf
```

```conf
# Archivo: config/mosquitto/mosquitto.conf
listener 1883
protocol mqtt
allow_anonymous false
password_file /mosquitto/config/passwd

listener 9001
protocol websockets

persistence true
persistence_location /mosquitto/data/

log_dest file /mosquitto/log/mosquitto.log
log_dest stdout
log_type all
log_timestamp true

# Configuración de seguridad
max_inflight_messages 20
max_queued_messages 100
message_size_limit 0
```

```bash
# Crear usuarios MQTT
$ docker run -it --rm -v $(pwd)/config/mosquitto:/mosquitto/config eclipse-mosquitto:2.0 mosquitto_passwd -c /mosquitto/config/passwd admin

# Agregar más usuarios
$ docker run -it --rm -v $(pwd)/config/mosquitto:/mosquitto/config eclipse-mosquitto:2.0 mosquitto_passwd /mosquitto/config/passwd nodered
$ docker run -it --rm -v $(pwd)/config/mosquitto:/mosquitto/config eclipse-mosquitto:2.0 mosquitto_passwd /mosquitto/config/passwd dispositivo01
```

#### 1.8 Levantar los Servicios
```bash
# Desde el directorio del proyecto
$ cd ~/iot-project
$ docker-compose up -d

# Verificar que todos los contenedores están corriendo
$ docker-compose ps

# Ver logs en tiempo real
$ docker-compose logs -f

# Ver logs de un servicio específico
$ docker-compose logs -f mosquitto
```

#### 1.9 Verificación de Servicios
```bash
# Verificar Mosquitto (MQTT)
$ docker exec -it mosquitto mosquitto_sub -h localhost -t test/topic -u admin -P [tu-contraseña]
# En otra terminal:
$ docker exec -it mosquitto mosquitto_pub -h localhost -t test/topic -m "Hola IoT" -u admin -P [tu-contraseña]

# Verificar Node-RED
# Abrir navegador: http://192.168.1.10:1880

# Verificar InfluxDB
# Abrir navegador: http://192.168.1.10:8086
# User: admin / Pass: adminpassword123

# Verificar Grafana
# Abrir navegador: http://192.168.1.10:3000
# User: admin / Pass: admin123
```

### ✅ Criterios de Éxito de Fase 1
- [ ] Raspberry Pi funcionando con IP estática
- [ ] Docker y Docker Compose instalados
- [ ] 4 contenedores corriendo sin errores
- [ ] MQTT pub/sub funcional con autenticación
- [ ] Acceso web a Node-RED, InfluxDB, Grafana

---

## 🔧 FASE 2: Primer Dispositivo IoT (Semana 3)

### Objetivo
Integrar un dispositivo real (Sonoff o ESP32) y lograr control básico + telemetría.

### Opción A: Sonoff con Tasmota (Más fácil)

#### 2.1 Hardware Necesario
```
- Sonoff Basic R2/R3 (~$8-12 USD)
- Adaptador USB-TTL para flashear (opcional)
```

#### 2.2 Instalación de Tasmota
```bash
# Opción 1: Via OTA (Over-The-Air) - Sin abrir el dispositivo
# 1. Conectar Sonoff a WiFi con app eWeLink
# 2. Usar herramienta Tasmota Convert
# Tutorial: https://tasmota.github.io/docs/Getting-Started/

# Opción 2: Flasheo manual via USB-TTL
# Tutorial: https://tasmota.github.io/docs/Getting-Started/#hardware-preparation
```

#### 2.3 Configuración de Tasmota
```
1. Conectar a WiFi "tasmota-XXXX"
2. Configurar WiFi de tu hogar
3. Configurar MQTT:
   - Host: 192.168.1.10
   - Port: 1883
   - User: dispositivo01
   - Password: [tu-contraseña]
   - Topic: sonoff_sala_luz1
   - Full Topic: %prefix%/%topic%/

4. Configurar Módulo:
   - Configuration → Configure Module
   - Seleccionar: Sonoff Basic (1)
   - Save
```

#### 2.4 Probar Comunicación MQTT
```bash
# Suscribirse a todos los tópicos del Sonoff
$ docker exec -it mosquitto mosquitto_sub -h localhost -t "sonoff_sala_luz1/#" -u admin -P [contraseña] -v

# Enviar comando ON
$ docker exec -it mosquitto mosquitto_pub -h localhost -t "cmnd/sonoff_sala_luz1/POWER" -m "ON" -u admin -P [contraseña]

# Enviar comando OFF
$ docker exec -it mosquitto mosquitto_pub -h localhost -t "cmnd/sonoff_sala_luz1/POWER" -m "OFF" -u admin -P [contraseña]

# Deberías ver en la suscripción:
# stat/sonoff_sala_luz1/RESULT {"POWER":"ON"}
# stat/sonoff_sala_luz1/POWER ON
```

### Opción B: ESP32 con Sensor (Más flexible)

#### 2.5 Hardware Necesario
```
- ESP32 DevKit (~$6-10 USD)
- Sensor DHT22 (temperatura + humedad) (~$5)
- Cables Dupont
- Protoboard
```

#### 2.6 Conexiones ESP32 + DHT22
```
DHT22 VCC  → ESP32 3.3V
DHT22 DATA → ESP32 GPIO4
DHT22 GND  → ESP32 GND
```

#### 2.7 Código ESP32 (PlatformIO)
```bash
# Instalar PlatformIO (desde VS Code Extensions)
# O via CLI:
$ pip install platformio

# Crear proyecto
$ mkdir esp32_sensor && cd esp32_sensor
$ platformio init --board esp32dev
```

```cpp
// Archivo: src/main.cpp
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// Configuración WiFi
const char* ssid = "TU_WIFI_SSID";
const char* password = "TU_WIFI_PASSWORD";

// Configuración MQTT
const char* mqtt_server = "192.168.1.10";
const int mqtt_port = 1883;
const char* mqtt_user = "dispositivo01";
const char* mqtt_password = "TU_PASSWORD_MQTT";
const char* mqtt_client_id = "ESP32_Sensor_01";

// Tópicos MQTT
const char* topic_temp = "home/sensores/esp32_01/tele/temp";
const char* topic_hum = "home/sensores/esp32_01/tele/humidity";
const char* topic_state = "home/sensores/esp32_01/state";
const char* topic_lwt = "home/sensores/esp32_01/lwt";

// Configuración DHT22
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;
const long interval = 30000;  // 30 segundos

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a WiFi: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    
    // Intentar conectar con LWT
    if (client.connect(mqtt_client_id, mqtt_user, mqtt_password,
                       topic_lwt, 1, true, "OFFLINE")) {
      Serial.println("Conectado!");
      
      // Publicar estado ONLINE
      client.publish(topic_lwt, "ONLINE", true);
      client.publish(topic_state, "{\"status\":\"connected\"}", true);
      
    } else {
      Serial.print("Fallo, rc=");
      Serial.print(client.state());
      Serial.println(" Reintentando en 5 segundos...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  // Inicializar sensor
  dht.begin();
  
  // Conectar WiFi
  setup_wifi();
  
  // Configurar MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setKeepAlive(60);
  
  reconnect();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > interval) {
    lastMsg = now;
    
    // Leer sensor
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    
    // Verificar lectura válida
    if (isnan(temp) || isnan(hum)) {
      Serial.println("Error leyendo sensor DHT22!");
      return;
    }
    
    // Publicar temperatura
    char tempStr[8];
    dtostrf(temp, 6, 2, tempStr);
    client.publish(topic_temp, tempStr);
    
    // Publicar humedad
    char humStr[8];
    dtostrf(hum, 6, 2, humStr);
    client.publish(topic_hum, humStr);
    
    // Log en Serial
    Serial.print("Temperatura: ");
    Serial.print(temp);
    Serial.print("°C | Humedad: ");
    Serial.print(hum);
    Serial.println("%");
  }
}
```

```ini
# Archivo: platformio.ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino

lib_deps = 
    knolleary/PubSubClient@^2.8
    adafruit/DHT sensor library@^1.4.4
    adafruit/Adafruit Unified Sensor@^1.1.9

monitor_speed = 115200
```

```bash
# Compilar y subir
$ platformio run --target upload

# Monitorear Serial
$ platformio device monitor
```

#### 2.8 Verificar Telemetría
```bash
# Suscribirse a telemetría del ESP32
$ docker exec -it mosquitto mosquitto_sub -h localhost -t "home/sensores/esp32_01/#" -u admin -P [contraseña] -v

# Deberías ver cada 30 segundos:
# home/sensores/esp32_01/lwt ONLINE
# home/sensores/esp32_01/tele/temp 23.50
# home/sensores/esp32_01/tele/humidity 45.20
```

### ✅ Criterios de Éxito de Fase 2
- [ ] Dispositivo conectado a MQTT
- [ ] Recepción de telemetría en tiempo real
- [ ] Control remoto funcional (si aplica)
- [ ] LWT operativo (ONLINE/OFFLINE)

---

## 📊 FASE 3: Almacenamiento y Visualización (Semana 4)

### Objetivo
Almacenar telemetría en InfluxDB y crear dashboard básico en Grafana.

### 3.1 Crear Flujo Node-RED: MQTT → InfluxDB

```bash
# Acceder a Node-RED: http://192.168.1.10:1880
```

#### 3.1.1 Instalar Nodos Adicionales
```
Menu (☰) → Manage palette → Install:
- node-red-contrib-influxdb
- node-red-dashboard (para UI)
```

#### 3.1.2 Crear Flujo de Ingesta
```json
[
  {
    "id": "mqtt_in",
    "type": "mqtt in",
    "name": "MQTT Telemetría",
    "topic": "home/+/+/tele/#",
    "qos": "1",
    "broker": "mqtt_broker",
    "x": 150,
    "y": 100
  },
  {
    "id": "function_parse",
    "type": "function",
    "name": "Parsear Topic",
    "func": "// Extraer zona, dispositivo, métrica del topic\n// Ej: home/sensores/esp32_01/tele/temp\nconst parts = msg.topic.split('/');\n\nif (parts.length >= 5) {\n  msg.zona = parts[1];\n  msg.dispositivo = parts[2];\n  msg.metrica = parts[4];\n  msg.payload = parseFloat(msg.payload);\n  \n  return msg;\n}\nreturn null;",
    "x": 350,
    "y": 100
  },
  {
    "id": "influx_out",
    "type": "influxdb out",
    "name": "InfluxDB Write",
    "influxdb": "influx_connection",
    "measurement": "{{metrica}}",
    "tags": "zona={{zona}},dispositivo={{dispositivo}}",
    "x": 550,
    "y": 100
  }
]
```

#### 3.1.3 Configurar Conexión InfluxDB
```
Doble click en nodo InfluxDB Out → Edit influxdb connection:

Version: 2.0
URL: http://influxdb:8086
Token: mi-token-super-secreto-12345
Organization: smarthome
Bucket: iot_data
```

### 3.2 Configurar Grafana

#### 3.2.1 Agregar Data Source
```
1. Acceder: http://192.168.1.10:3000
2. Login: admin / admin123
3. Configuration (⚙️) → Data sources → Add data source
4. Seleccionar: InfluxDB
5. Configurar:
   - Query Language: Flux
   - URL: http://influxdb:8086
   - Organization: smarthome
   - Token: mi-token-super-secreto-12345
   - Default Bucket: iot_data
6. Save & Test
```

#### 3.2.2 Crear Dashboard Básico
```
1. Create (+) → Dashboard → Add new panel
2. Query (Flux):

from(bucket: "iot_data")
  |> range(start: -1h)
  |> filter(fn: (r) => r._measurement == "temp")
  |> filter(fn: (r) => r._field == "value")

3. Panel settings:
   - Title: Temperatura (Última Hora)
   - Visualization: Time series
   - Unit: Celsius (°C)
   
4. Repetir para humedad:

from(bucket: "iot_data")
  |> range(start: -1h)
  |> filter(fn: (r) => r._measurement == "humidity")
  |> filter(fn: (r) => r._field == "value")
```

### ✅ Criterios de Éxito de Fase 3
- [ ] Datos fluyendo MQTT → Node-RED → InfluxDB
- [ ] Dashboard Grafana mostrando telemetría en tiempo real
- [ ] Gráficos históricos de al menos 1 hora

---

## 🎮 FASE 4: Control Básico (Semana 5)

### Objetivo
Implementar control bidireccional: UI → MQTT → Dispositivo → Estado → UI

### 4.1 Crear Panel de Control en Node-RED Dashboard

```json
[
  {
    "id": "ui_switch",
    "type": "ui_switch",
    "name": "Luz Sala",
    "topic": "home/sala/luz1/cmd",
    "payload": "ON",
    "payloadOff": "OFF",
    "x": 150,
    "y": 200
  },
  {
    "id": "mqtt_cmd_out",
    "type": "mqtt out",
    "name": "Enviar Comando",
    "topic": "",
    "qos": "1",
    "broker": "mqtt_broker",
    "x": 350,
    "y": 200
  },
  {
    "id": "mqtt_state_in",
    "type": "mqtt in",
    "name": "Recibir Estado",
    "topic": "home/sala/luz1/state",
    "qos": "1",
    "broker": "mqtt_broker",
    "x": 150,
    "y": 300
  },
  {
    "id": "function_state",
    "type": "function",
    "name": "Parse Estado",
    "func": "msg.payload = (msg.payload === 'ON');\nreturn msg;",
    "x": 350,
    "y": 300
  }
]
```

### 4.2 Acceder al Dashboard
```
URL: http://192.168.1.10:1880/ui
```

### ✅ Criterios de Éxito de Fase 4
- [ ] Botón ON/OFF funcional en Node-RED Dashboard
- [ ] Estado sincronizado en tiempo real
- [ ] Latencia < 200ms

---

## 🤖 FASE 5: Primera Escena Automatizada (Semana 6)

### Objetivo
Implementar escena de ahorro energético básica.

### 5.1 Escena: Apagado Automático por Tiempo

```json
// Node-RED Flow: Apagar luz después de 30 minutos sin movimiento
[
  {
    "id": "inject_timer",
    "type": "inject",
    "name": "Timer 30min",
    "repeat": "1800",  // 30 minutos
    "crontab": "",
    "once": false,
    "topic": ""
  },
  {
    "id": "function_check_state",
    "type": "function",
    "name": "Verificar Estado ON",
    "func": "// Consultar si la luz está encendida\n// Si está ON, enviar comando OFF\nreturn msg;"
  },
  {
    "id": "mqtt_auto_off",
    "type": "mqtt out",
    "name": "Apagar Automático",
    "topic": "home/sala/luz1/cmd",
    "payload": "OFF",
    "qos": "1"
  }
]
```

### ✅ Criterios de Éxito de Fase 5
- [ ] Escena ejecutándose automáticamente
- [ ] Notificación de acción (log o mensaje)
- [ ] Evento almacenado en InfluxDB

---

## 📦 Estructura de Archivos Final

```
~/iot-project/
├── docker-compose.yml
├── .env
├── .gitignore
├── README.md
├── config/
│   ├── mosquitto/
│   │   ├── mosquitto.conf
│   │   ├── passwd
│   │   └── acl
│   ├── nodered/
│   │   └── flows.json
│   └── grafana/
│       └── provisioning/
│           └── datasources/
│               └── influxdb.yml
├── firmware/
│   └── esp32_sensor/
│       ├── src/
│       │   └── main.cpp
│       └── platformio.ini
├── scripts/
│   ├── backup.sh
│   ├── restore.sh
│   └── monitor.sh
└── docs/
    ├── diagramas.md
    └── DESARROLLO.md
```

---

## 🔐 Checklist de Seguridad

```bash
# 1. Cambiar contraseñas por defecto
- [ ] Grafana admin
- [ ] InfluxDB admin
- [ ] Usuarios MQTT
- [ ] Raspberry Pi (pi)

# 2. Configurar firewall
$ sudo apt install ufw -y
$ sudo ufw allow 22/tcp      # SSH
$ sudo ufw allow 1883/tcp    # MQTT
$ sudo ufw allow from 192.168.1.0/24 to any port 3000  # Grafana (solo LAN)
$ sudo ufw allow from 192.168.1.0/24 to any port 1880  # Node-RED (solo LAN)
$ sudo ufw enable

# 3. Backups automáticos
$ crontab -e
# Agregar:
0 2 * * * ~/iot-project/scripts/backup.sh
```

---

## 📊 Matriz de Prioridades

| Tarea | Prioridad | Dificultad | Tiempo | Bloqueante |
|-------|-----------|------------|--------|------------|
| Instalar Raspberry Pi OS | 🔴 Alta | Baja | 1h | Sí |
| Docker + Docker Compose | 🔴 Alta | Baja | 30min | Sí |
| Levantar servicios | 🔴 Alta | Media | 2h | Sí |
| Configurar MQTT | 🔴 Alta | Media | 1h | Sí |
| Integrar 1er dispositivo | 🟡 Media | Media | 3h | No |
| Flujo Node-RED → InfluxDB | 🟡 Media | Media | 2h | No |
| Dashboard Grafana | 🟢 Baja | Baja | 1h | No |
| Escena automatizada | 🟢 Baja | Alta | 3h | No |

---

## 🚦 Próximos Pasos Inmediatos

### Semana 1 (AHORA)
1. ✅ Comprar Raspberry Pi 5 + accesorios
2. ✅ Instalar Raspberry Pi OS 64-bit
3. ✅ Configurar IP estática
4. ✅ Instalar Docker + Docker Compose
5. ✅ Levantar 4 servicios básicos

### Semana 2
6. ✅ Configurar autenticación MQTT
7. ✅ Probar pub/sub con mosquitto_pub/sub
8. ✅ Acceder a todas las UIs web

### Semana 3
9. ✅ Flashear Sonoff con Tasmota O programar ESP32
10. ✅ Integrar primer dispositivo a MQTT

### Semana 4
11. ✅ Crear flujo Node-RED de ingesta
12. ✅ Configurar InfluxDB datasource en Grafana
13. ✅ Crear primer dashboard

---

## 📚 Recursos Recomendados

### Documentación Oficial
- [Raspberry Pi Documentation](https://www.raspberrypi.com/documentation/)
- [Docker Compose](https://docs.docker.com/compose/)
- [Mosquitto](https://mosquitto.org/documentation/)
- [Node-RED Guide](https://nodered.org/docs/getting-started/)
- [InfluxDB 2.x](https://docs.influxdata.com/influxdb/v2/)
- [Grafana Tutorials](https://grafana.com/tutorials/)

### Tutoriales Específicos
- [Tasmota Installation](https://tasmota.github.io/docs/Getting-Started/)
- [ESP32 + MQTT + DHT22](https://randomnerdtutorials.com/esp32-mqtt-publish-dht11-dht22-arduino/)
- [Node-RED Dashboard](https://flows.nodered.org/node/node-red-dashboard)

### Comunidades
- [Home Assistant Community](https://community.home-assistant.io/)
- [Node-RED Forum](https://discourse.nodered.org/)
- [Raspberry Pi Forums](https://forums.raspberrypi.com/)

---

## ⚠️ Problemas Comunes y Soluciones

### Problema: Contenedores no inician
```bash
# Ver logs detallados
$ docker-compose logs [servicio]

# Verificar puertos en uso
$ sudo netstat -tulpn | grep LISTEN

# Reiniciar todos los servicios
$ docker-compose down && docker-compose up -d
```

### Problema: ESP32 no conecta a WiFi
```cpp
// Agregar debug en Serial
Serial.println(WiFi.status());
// 0 = WL_IDLE_STATUS
// 3 = WL_CONNECTED
// 4 = WL_CONNECT_FAILED
```

### Problema: MQTT "Connection Refused"
```bash
# Verificar credenciales
$ docker exec -it mosquitto cat /mosquitto/config/passwd

# Test de conexión
$ docker exec -it mosquitto mosquitto_pub -h localhost -t test -m "hola" -u admin -P [pass] -d
```

---

## 🎯 Objetivo Final: MVP (Minimum Viable Product)

Al completar las 5 fases, tendrás:

✅ Raspberry Pi 5 con stack completo (MQTT, Node-RED, InfluxDB, Grafana)  
✅ Al menos 1 dispositivo IoT integrado (Sonoff o ESP32)  
✅ Telemetría almacenada en time-series DB  
✅ Dashboard visual en tiempo real  
✅ Control bidireccional funcional  
✅ 1 escena automatizada operativa  

**Esto es una base sólida para expandir a 10-20 dispositivos y escenas más complejas.**

---

## 📧 Soporte

¿Problemas durante el desarrollo?

1. Revisar logs: `docker-compose logs -f [servicio]`
2. Consultar documentación oficial
3. Buscar en foros de la comunidad
4. Crear issue en GitHub si es específico del proyecto

---

**¡Éxito con el desarrollo! 🚀**
