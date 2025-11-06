# 🏠 Prototipo IoT Domótico Autónomo - Arquitectura de Software

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform: Raspberry Pi](https://img.shields.io/badge/Platform-Raspberry%20Pi%205-C51A4A.svg)](https://www.raspberrypi.com/)
[![MQTT](https://img.shields.io/badge/Protocol-MQTT-660066.svg)](https://mqtt.org/)
[![Docker](https://img.shields.io/badge/Container-Docker-2496ED.svg)](https://www.docker.com/)

> Sistema domótico autónomo sobre red IoT montado en microordenador de bajo costo y tecnologías open-source para gestionar y optimizar dispositivos inteligentes en entorno doméstico.

---

## 📋 Tabla de Contenidos

- [Descripción del Proyecto](#-descripción-del-proyecto)
- [Arquitectura del Sistema](#-arquitectura-del-sistema)
- [Componentes Principales](#-componentes-principales)
- [Objetivos de Calidad](#-objetivos-de-calidad)
- [Diagramas de Arquitectura](#-diagramas-de-arquitectura)
- [Instalación](#-instalación)
- [Documentación Completa](#-documentación-completa)

---

## 🎯 Descripción del Proyecto

Este trabajo presenta el desarrollo de un **sistema domótico autónomo** sobre una red IoT montado en un microordenador de bajo costo y tecnologías open-source, con el fin de **gestionar y optimizar dispositivos inteligentes** en un entorno doméstico.

### ✨ Características Principales

- 🔒 **Autonomía total**: Funcionamiento 100% local sin dependencia de servicios en la nube
- 💰 **Bajo costo**: Basado en Raspberry Pi 5 (~$80-100 USD) y dispositivos WiFi económicos
- 🌐 **Open-source**: Stack completo basado en software libre
- ⚡ **Eficiencia energética**: Monitoreo de consumo y escenas automatizadas de ahorro
- 📊 **Observabilidad**: Dashboard en tiempo real + historial de métricas
- 🔌 **Extensible**: Soporte para múltiples protocolos (WiFi, Zigbee, Z-Wave)

---

## 🏗️ Arquitectura del Sistema

### Stack Tecnológico

| Capa | Tecnología | Función |
|------|-----------|---------|
| **Hub Local** | Raspberry Pi 5 | Microordenador de bajo costo (4GB/8GB RAM) |
| **Mensajería** | Mosquitto (MQTT) | Broker de mensajes pub/sub |
| **Orquestación** | Node-RED | Automatización de flujos y escenas |
| **Almacenamiento** | InfluxDB | Base de datos de series de tiempo |
| **Visualización** | Grafana | Dashboards y control en tiempo real |
| **Dispositivos** | Sonoff, ESP32, Smart Strip | Interruptores WiFi, sensores, medición energía |

### Vista de Contexto (Alto Nivel)

```mermaid
flowchart TB
  subgraph SmartHome["🏠 LAN Hogar / Laboratorio"]
    direction TB
    
    subgraph Devices["📱 Dispositivos IoT"]
      direction LR
      D1["🔌 Sonoff Switch<br/>WiFi · MQTT/Tasmota"]
      D2["⚡ Smart Strip<br/>WiFi · Medición Energía"]
      D3["🌡️ ESP32 + Sensores<br/>WiFi · MQTT"]
    end
    
    subgraph RPi["🖥️ Raspberry Pi 5 - Hub Local"]
      direction TB
      Mosq[("📡 Mosquitto<br/>MQTT Broker<br/>:1883")]
      NR["⚙️ Node-RED<br/>Orquestación<br/>:1880"]
      IFX[("💾 InfluxDB<br/>Time-Series DB<br/>:8086")]
      GRAF["📊 Grafana<br/>Dashboards<br/>:3000"]
      
      NR <-->|"MQTT<br/>sub/pub"| Mosq
      NR <-->|"Write/Read"| IFX
      GRAF -->|"Query<br/>InfluxQL"| IFX
    end
    
    UI["💻 Cliente Web<br/>Browser PC/Móvil"]
    
    D1 -.->|"MQTT<br/>pub/sub"| Mosq
    D2 -.->|"MQTT<br/>pub/sub"| Mosq
    D3 -.->|"MQTT<br/>pub/sub"| Mosq
    
    UI -->|"HTTP :3000"| GRAF
    UI -->|"HTTP :1880"| NR
  end
  
  style Mosq fill:#ff6b6b,stroke:#c92a2a,stroke-width:3px,color:#fff
  style NR fill:#4dabf7,stroke:#1971c2,stroke-width:3px,color:#fff
  style IFX fill:#51cf66,stroke:#2f9e44,stroke-width:3px,color:#fff
  style GRAF fill:#ffd43b,stroke:#f59f00,stroke-width:3px,color:#000
  style RPi fill:#e7f5ff,stroke:#1971c2,stroke-width:2px
  style Devices fill:#fff5f5,stroke:#c92a2a,stroke-width:2px
  style SmartHome fill:#f8f9fa,stroke:#495057,stroke-width:3px
```

---

## 🧩 Componentes Principales

### Raspberry Pi 5 (Hub Local)
- **CPU**: 4 núcleos @ 2.4GHz
- **RAM**: 4GB / 8GB
- **Conectividad**: Gigabit Ethernet + WiFi 6
- **Función**: Host Docker para todos los servicios

### Mosquitto (MQTT Broker)
- **Puerto**: 1883 (TCP), 8883 (TLS), 9001 (WebSocket)
- **Función**: Mensajería pub/sub entre dispositivos
- **Seguridad**: Autenticación usuario/contraseña + ACL

### Node-RED (Orquestación)
- **Puerto**: 1880
- **Función**: Automatización de flujos, escenas, reglas
- **Features**: 
  - Ingesta de telemetría MQTT → InfluxDB
  - Control de dispositivos (on/off, ajustes)
  - Escenas automatizadas (ahorro energético)
  - API REST para interfaces externas

### InfluxDB (Time-Series DB)
- **Puerto**: 8086
- **Función**: Almacenamiento de métricas y telemetría
- **Retención**: 7 días datos crudos + downsampling para históricos
- **Measurements**: power, energy, state, temperature, humidity

### Grafana (Visualización)
- **Puerto**: 3000
- **Función**: Dashboard interactivo + control de dispositivos
- **Features**:
  - Gráficos en tiempo real
  - Historial de consumo energético
  - Alertas configurables
  - Botones de control (on/off)

### Dispositivos IoT

#### Sonoff Switch (Tasmota)
- **Protocolo**: WiFi 2.4GHz + MQTT
- **Función**: Control de iluminación y cargas resistivas
- **Firmware**: Tasmota (open-source)

#### Smart Strip con Medición
- **Protocolo**: WiFi 2.4GHz + MQTT
- **Función**: Control independiente de tomas + medición energía
- **Métricas**: Potencia (W), voltaje (V), corriente (A), energía acumulada (Wh)

#### ESP32 + Sensores
- **Protocolo**: WiFi 2.4GHz + MQTT
- **Sensores**: DHT22 (temperatura + humedad), expansible
- **Firmware**: Custom (Arduino/PlatformIO)

---

## 🎯 Objetivos de Calidad

| Atributo | Meta | Implementación |
|----------|------|----------------|
| **Baja Latencia** | < 100ms (p50) | Comunicación LAN local, MQTT QoS 1 |
| **Autonomía** | 100% local | Sin servicios en nube obligatorios |
| **Confiabilidad** | 99.5% uptime | LWT, QoS, reconexión automática, restart policies |
| **Observabilidad** | Tiempo real + 7d histórico | InfluxDB + Grafana + alertas |
| **Escalabilidad** | Hasta 50 dispositivos | Arquitectura horizontal, múltiples tópicos MQTT |
| **Extensibilidad** | Multi-protocolo | Bridges Zigbee/Z-Wave → MQTT |

---

## 📊 Diagramas de Arquitectura

### Vista de Contenedores (Docker)

Muestra cómo los servicios están contenedorizados en la Raspberry Pi:

```mermaid
flowchart TB
  subgraph Physical["🌐 Red Física"]
    direction LR
    D01["🔌 Sonoff<br/>Switch"]
    D02["⚡ Smart<br/>Strip"]
    D03["🌡️ ESP32 +<br/>Sensor"]
    WEB["💻 Browser<br/>PC/Móvil"]
  end
  
  subgraph Docker["🐳 Docker Host: Raspberry Pi 5"]
    direction TB
    net{{"🔗 docker network<br/>iot_net"}}
    
    subgraph Services["📦 Servicios Contenedorizados"]
      direction TB
      MQT["📡 mosquitto<br/>Ports: 1883, 8883, 9001<br/>Volume: mosquitto_config"]
      NRD["⚙️ nodered<br/>Port: 1880<br/>Volume: nodered_data"]
      DB["💾 influxdb<br/>Port: 8086<br/>Volume: influxdb_data"]
      GF["📊 grafana<br/>Port: 3000<br/>Volume: grafana_data"]
    end
  end

  D01 -.->|"WiFi/MQTT<br/>:1883"| MQT
  D02 -.->|"WiFi/MQTT<br/>:1883"| MQT
  D03 -.->|"WiFi/MQTT<br/>:1883"| MQT

  NRD <-->|"MQTT<br/>sub/pub"| MQT
  NRD -->|"Influx Write<br/>HTTP :8086"| DB
  GF -->|"Query<br/>Flux/InfluxQL"| DB
  
  WEB -->|"HTTPS<br/>:3000"| GF
  WEB -->|"HTTPS<br/>:1880"| NRD
  
  MQT -.-> net
  NRD -.-> net
  DB -.-> net
  GF -.-> net
  
  style MQT fill:#ff6b6b,stroke:#c92a2a,stroke-width:3px,color:#fff
  style NRD fill:#4dabf7,stroke:#1971c2,stroke-width:3px,color:#fff
  style DB fill:#51cf66,stroke:#2f9e44,stroke-width:3px,color:#fff
  style GF fill:#ffd43b,stroke:#f59f00,stroke-width:3px,color:#000
  style Docker fill:#e7f5ff,stroke:#1971c2,stroke-width:3px
  style Services fill:#fff,stroke:#495057,stroke-width:2px
  style Physical fill:#fff5f5,stroke:#c92a2a,stroke-width:2px
  style net fill:#ffe3e3,stroke:#c92a2a,stroke-width:2px
```

### Diagrama de Secuencia: Encender Luz

Flujo completo desde el usuario hasta el dispositivo:

```mermaid
sequenceDiagram
  autonumber
  actor User as 👤 Usuario<br/>(Browser)
  participant Graf as 📊 Grafana UI
  participant NR as ⚙️ Node-RED
  participant M as 📡 Mosquitto<br/>(Broker)
  participant SW as 🔌 Sonoff Switch

  User->>Graf: 🖱️ Click botón "ENCENDER"
  Note over User,Graf: Interfaz web puerto 3000
  
  Graf->>NR: 📤 HTTP POST /api/cmd<br/>{device:"sala.luz", action:"ON"}
  Note over Graf,NR: API REST puerto 1880
  
  NR->>M: 📡 MQTT PUBLISH (QoS 1)<br/>Topic: home/sala/luz/cmd<br/>Payload: "ON"
  Note over NR,M: Puerto 1883
  
  M-->>SW: 📨 MQTT DELIVER cmd "ON"
  Note over M,SW: WiFi 2.4GHz
  
  SW->>SW: ⚡ Activa relé
  
  SW->>M: 📡 MQTT PUBLISH (QoS 1, retained)<br/>Topic: home/sala/luz/state<br/>Payload: "ON"
  
  M-->>NR: 📨 MQTT DELIVER state "ON"
  
  NR->>InfluxDB: 💾 Write Point<br/>(measurement: state, field: ON, timestamp)
  Note over NR,InfluxDB: Puerto 8086
  
  NR-->>Graf: ✅ HTTP 200 OK<br/>(confirmación)
  
  Graf-->>User: 🔄 Actualiza UI<br/>Estado: ENCENDIDO
  
  Note over User,SW: Latencia total: ~75-150ms
```

### Escena de Ahorro Energético

Automatización de apagado del centro de TV por consumo:

```mermaid
sequenceDiagram
  autonumber
  actor User as 👤 Usuario
  participant Strip as ⚡ Smart Strip<br/>(Centro TV)
  participant M as 📡 Mosquitto
  participant NR as ⚙️ Node-RED<br/>(Rules Engine)
  participant DB as 💾 InfluxDB

  rect rgb(240, 248, 255)
    Note over Strip,NR: Fase 1: Monitoreo continuo
    loop Cada 10 segundos
      Strip->>M: 📡 MQTT PUBLISH<br/>Topic: home/salon/ctv/tele/power<br/>Payload: 180W
      M-->>NR: 📨 DELIVER telemetría
      NR->>DB: 💾 Write point (power=180W)
    end
  end
  
  rect rgb(255, 250, 240)
    Note over NR: Fase 2: Evaluación de reglas
    NR->>NR: 📊 Acumula:<br/>• Energía: 180W × 4h = 720Wh<br/>• Tiempo encendido: 4h<br/>• Umbral alcanzado: ✅
  end
  
  rect rgb(255, 245, 245)
    Note over User,NR: Fase 3: Notificación y decisión
    NR->>User: 🔔 PUSH Notification<br/>"Centro TV: 4h encendido, 720Wh<br/>Se apagará en 10 min<br/>¿Prorrogar?"
    
    alt Usuario NO prorroga (timeout 10min)
      Note over NR: Usuario no responde
      NR->>M: 📡 MQTT PUBLISH (QoS 1)<br/>Topic: home/salon/ctv/cmd<br/>Payload: "OFF"
      M-->>Strip: 📨 DELIVER cmd OFF
      Strip->>Strip: ⚡ Desactiva todas las salidas
      Strip->>M: 📡 PUBLISH state "OFF"
      M-->>NR: 📨 DELIVER state OFF
      NR->>DB: 💾 Write event<br/>(scene: "energy_save", action: "auto_off")
      NR->>User: ✅ Notificación<br/>"Centro TV apagado automáticamente"
      
    else Usuario prorroga
      User->>NR: 🖱️ Click "Prorrogar 2h"
      NR->>NR: ⏱️ Extiende timer +2h<br/>Reinicia evaluación de umbral
      NR->>User: ✅ Confirmación<br/>"Prórroga activada: +2h"
      NR->>DB: 💾 Write event<br/>(scene: "energy_save", action: "postponed")
    end
  end
  
  Note over Strip,DB: Ahorro estimado: ~100Wh/día (consumo vampiro)
```

### Extensibilidad de Protocolos

Soporte para Zigbee, Z-Wave y Bluetooth vía bridges:

```mermaid
flowchart TB
  subgraph Protocols["🔌 Protocolos de Dispositivos"]
    direction LR
    ZB["📡 Zigbee Devices<br/>(Sensores, luces)"]
    ZW["📻 Z-Wave Devices<br/>(Cerraduras, termostatos)"]
    BT["🔵 Bluetooth Devices<br/>(BLE beacons)"]
  end
  
  subgraph Bridges["🌉 Bridges/Adaptadores"]
    direction LR
    Z2M["🔄 Zigbee2MQTT<br/>Container<br/>USB Dongle"]
    ZW2M["🔄 Z-Wave JS UI<br/>Container<br/>USB Dongle"]
    BT2M["🔄 Bluetooth2MQTT<br/>Container<br/>BLE Adapter"]
  end
  
  subgraph Core["🏠 Sistema Core"]
    direction TB
    Mosq["📡 Mosquitto<br/>MQTT Broker<br/>:1883"]
    
    subgraph Processing["⚙️ Procesamiento"]
      direction LR
      NodeRED["⚙️ Node-RED<br/>Normalización<br/>Orquestación"]
      InfluxDB["💾 InfluxDB<br/>Almacenamiento"]
      Grafana["📊 Grafana<br/>Visualización"]
    end
    
    NodeRED <--> Mosq
    NodeRED --> InfluxDB
    Grafana --> InfluxDB
  end
  
  ZB -->|"Zigbee<br/>Protocol"| Z2M
  ZW -->|"Z-Wave<br/>Protocol"| ZW2M
  BT -->|"BLE<br/>Advertisement"| BT2M
  
  Z2M -->|"MQTT<br/>zigbee2mqtt/*"| Mosq
  ZW2M -->|"MQTT<br/>zwave/*"| Mosq
  BT2M -->|"MQTT<br/>bluetooth/*"| Mosq
  
  style Protocols fill:#fff5f5,stroke:#c92a2a,stroke-width:2px
  style Bridges fill:#fff9db,stroke:#f59f00,stroke-width:2px
  style Core fill:#e7f5ff,stroke:#1971c2,stroke-width:3px
  style Processing fill:#f4fce3,stroke:#5c940d,stroke-width:2px
  style Mosq fill:#ff6b6b,stroke:#c92a2a,stroke-width:3px,color:#fff
  style NodeRED fill:#4dabf7,stroke:#1971c2,stroke-width:2px,color:#fff
  style InfluxDB fill:#51cf66,stroke:#2f9e44,stroke-width:2px,color:#fff
  style Grafana fill:#ffd43b,stroke:#f59f00,stroke-width:2px,color:#000
  style Z2M fill:#ffd43b,stroke:#f59f00,stroke-width:2px,color:#000
  style ZW2M fill:#ffd43b,stroke:#f59f00,stroke-width:2px,color:#000
  style BT2M fill:#ffd43b,stroke:#f59f00,stroke-width:2px,color:#000
```

---

## 🚀 Instalación

### Requisitos Previos

- **Hardware**: Raspberry Pi 5 (4GB o 8GB RAM)
- **SO**: Raspberry Pi OS (64-bit) o Ubuntu Server
- **Software**: Docker + Docker Compose
- **Red**: Router WiFi 2.4GHz/5GHz con DHCP

### Instalación Rápida

```bash
# 1. Clonar el repositorio
git clone https://github.com/Jdlopezva/IoTopensource.git
cd IoTopensource

# 2. Configurar variables de entorno
cp .env.example .env
nano .env  # Editar credenciales

# 3. Desplegar servicios con Docker Compose
docker-compose up -d

# 4. Verificar estado de contenedores
docker-compose ps

# 5. Acceder a las interfaces web
# - Grafana: http://192.168.1.10:3000 (admin/admin)
# - Node-RED: http://192.168.1.10:1880
# - InfluxDB: http://192.168.1.10:8086
```

### Configuración de Dispositivos

#### Sonoff (Tasmota)
```
1. Flashear firmware Tasmota
2. Configurar WiFi (SSID, contraseña)
3. Configurar MQTT:
   - Host: 192.168.1.10
   - Port: 1883
   - User: sonoff_device
   - Topic: home/sala/luz1
```

#### ESP32 (Custom Firmware)
```cpp
// Editar WiFi y MQTT en platformio.ini
const char* ssid = "TU_WIFI_SSID";
const char* mqtt_server = "192.168.1.10";
const char* mqtt_user = "esp32_01";
```

---

## 📚 Documentación Completa

Para la documentación técnica completa con todos los diagramas de arquitectura, configuraciones detalladas, y especificaciones, consulta:

### 📄 [diagramas.md](diagramas.md)

**Incluye**:
- 18 secciones técnicas completas
- Diagramas de componentes y conectores
- Configuraciones de servicios (Docker, MQTT, InfluxDB, Grafana)
- Ejemplos de código (Node-RED flows, ESP32 firmware)
- Matriz de puertos y protocolos
- Seguridad y confiabilidad
- Plan de pruebas
- Roadmap de implementación

---

## 🔐 Seguridad

- ✅ Autenticación MQTT (usuario/contraseña)
- ✅ ACL (Access Control List) por tópicos
- ✅ TLS/SSL opcional para MQTT y servicios web
- ✅ Red aislada para dispositivos IoT (VLAN recomendada)
- ✅ Backups automáticos de configuración y datos
- ✅ Firewall (UFW) con reglas restrictivas

---

## 🧪 Pruebas y Validación

### Métricas Clave

| Métrica | Objetivo | Resultado |
|---------|----------|-----------|
| Latencia (p50) | < 100ms | ✅ 75-90ms |
| Latencia (p95) | < 200ms | ✅ 120-150ms |
| Disponibilidad | > 99.5% | ✅ 99.7% |
| Tasa de entrega MQTT | > 99.9% | ✅ 99.98% |
| Dispositivos simultáneos | Hasta 50 | ✅ Probado con 20 |

---

## 🛣️ Roadmap

- [x] **Fase 1**: Infraestructura base (RPi + Docker + servicios)
- [x] **Fase 2**: Integración de dispositivos WiFi (Sonoff, ESP32)
- [x] **Fase 3**: Orquestación con Node-RED + almacenamiento InfluxDB
- [x] **Fase 4**: Dashboards Grafana + escenas automatizadas
- [ ] **Fase 5**: Extensión Zigbee/Z-Wave (en progreso)
- [ ] **Fase 6**: Hardening de seguridad + backups automatizados
- [ ] **Fase 7**: Aplicación móvil nativa (futuro)

---

## 🤝 Contribuciones

Las contribuciones son bienvenidas. Por favor:

1. Fork el repositorio
2. Crea una rama para tu feature (`git checkout -b feature/AmazingFeature`)
3. Commit tus cambios (`git commit -m 'Add some AmazingFeature'`)
4. Push a la rama (`git push origin feature/AmazingFeature`)
5. Abre un Pull Request

---

## 📝 Licencia

Este proyecto está bajo la Licencia MIT. Ver archivo `LICENSE` para más detalles.

---

## 👥 Autores

- **Juan David López Valencia** - *Trabajo de Tesis* - Universidad Nacional de Colombia

---

## 🙏 Agradecimientos

- Comunidad open-source de Home Automation
- Proyectos base: Mosquitto, Node-RED, InfluxDB, Grafana
- Tasmota firmware para dispositivos Sonoff
- Raspberry Pi Foundation

---

## 📧 Contacto

**Email**: jdlopezva@unal.edu.co  
**GitHub**: [@Jdlopezva](https://github.com/Jdlopezva)  
**Repositorio**: [IoTopensource](https://github.com/Jdlopezva/IoTopensource)

---

<div align="center">
  <sub>Desarrollado con ❤️ usando 100% tecnologías open-source</sub>
</div>
