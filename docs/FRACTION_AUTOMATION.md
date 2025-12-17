# 🤖 Автоматизация управления фракциями

Примеры интеграции детектора фракций Smart Areometr с системами управления приемниками.

## 📋 Оглавление
- [Концепция](#концепция)
- [Arduino/ESP32 контроллер](#arduinoesp32-контроллер)
- [Raspberry Pi + Python](#raspberry-pi--python)
- [Node-RED автоматизация](#node-red-автоматизация)
- [Home Assistant](#home-assistant)
- [PLC/Промышленные контроллеры](#plcпромышленные-контроллеры)

---

## 🎯 Концепция

Smart Areometr определяет текущую фракцию и публикует события в MQTT. Внешняя автоматика подписывается на эти события и управляет клапанами/приемниками.

### Топология системы

```
┌─────────────────────┐
│  Smart Areometr     │
│  (определяет        │
│   фракции)          │
└──────────┬──────────┘
           │ MQTT
           ▼
┌─────────────────────┐
│   MQTT Broker       │
│   (Mosquitto)       │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Контроллер         │
│  автоматики         │
│  (управляет         │
│   клапанами)        │
└──────────┬──────────┘
           │
           ▼
     Сервоприводы/
      Клапаны
```

### MQTT события

**Топик:** `distillery/areometer/fraction`

**Payload (JSON):**
```json
{
  "event": "fraction_change",
  "from": "heads",
  "to": "body",
  "alcohol": 82.5,
  "temperature": 20.3,
  "volume": 250,
  "timestamp": 1234567890
}
```

**Типы фракций:**
- `foreshots` - Первач (метанол)
- `heads` - Головы (ацетон, эфиры)
- `body` - Тело (питьевая часть)
- `tails` - Хвосты (сивушные масла)
- `finished` - Завершено

---

## 🔧 Arduino/ESP32 контроллер

### Подключение сервоприводов

```cpp
/*
 * ESP32 контроллер управления приемниками
 * Подписывается на события фракций и переключает сервоприводы
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

// Wi-Fi
const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASSWORD";

// MQTT
const char* mqtt_server = "192.168.1.100";
const char* mqtt_topic = "distillery/areometer/fraction";

// Сервоприводы
#define SERVO_FORESHOTS_PIN 12
#define SERVO_HEADS_PIN 13
#define SERVO_BODY_PIN 14
#define SERVO_TAILS_PIN 27

Servo servoForeshots;
Servo servoHeads;
Servo servoBody;
Servo servoTails;

WiFiClient espClient;
PubSubClient mqtt(espClient);

// Углы открытия/закрытия
#define ANGLE_OPEN 90
#define ANGLE_CLOSE 0

void setup() {
    Serial.begin(115200);

    // Инициализация сервоприводов
    servoForeshots.attach(SERVO_FORESHOTS_PIN);
    servoHeads.attach(SERVO_HEADS_PIN);
    servoBody.attach(SERVO_BODY_PIN);
    servoTails.attach(SERVO_TAILS_PIN);

    // Закрываем все клапаны
    closeAllValves();

    // Подключение к Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");

    // Подключение к MQTT
    mqtt.setServer(mqtt_server, 1883);
    mqtt.setCallback(mqttCallback);

    connectMQTT();
}

void loop() {
    if (!mqtt.connected()) {
        connectMQTT();
    }
    mqtt.loop();
}

void connectMQTT() {
    while (!mqtt.connected()) {
        Serial.print("Connecting to MQTT...");
        if (mqtt.connect("ReceiverController")) {
            Serial.println(" connected!");
            mqtt.subscribe(mqtt_topic);
        } else {
            Serial.print(" failed, rc=");
            Serial.println(mqtt.state());
            delay(5000);
        }
    }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    // Парсим JSON
    JsonDocument doc;
    deserializeJson(doc, payload, length);

    String fraction = doc["to"] | "";
    float alcohol = doc["alcohol"] | 0.0f;

    Serial.printf("Fraction change to: %s (%.1f%%)\n", fraction.c_str(), alcohol);

    // Переключаем клапаны
    switchToFraction(fraction);
}

void switchToFraction(String fraction) {
    closeAllValves();
    delay(500); // Даем время клапану закрыться

    if (fraction == "foreshots") {
        openValve(servoForeshots);
        Serial.println("→ Opened FORESHOTS valve");
    }
    else if (fraction == "heads") {
        openValve(servoHeads);
        Serial.println("→ Opened HEADS valve");
    }
    else if (fraction == "body") {
        openValve(servoBody);
        Serial.println("→ Opened BODY valve");
    }
    else if (fraction == "tails") {
        openValve(servoTails);
        Serial.println("→ Opened TAILS valve");
    }
    else if (fraction == "finished") {
        closeAllValves();
        Serial.println("→ All valves CLOSED (finished)");
    }
}

void openValve(Servo &servo) {
    servo.write(ANGLE_OPEN);
}

void closeValve(Servo &servo) {
    servo.write(ANGLE_CLOSE);
}

void closeAllValves() {
    servoForeshots.write(ANGLE_CLOSE);
    servoHeads.write(ANGLE_CLOSE);
    servoBody.write(ANGLE_CLOSE);
    servoTails.write(ANGLE_CLOSE);
}
```

### Схема подключения

```
ESP32           Сервоприводы
GPIO 12  ────► Servo 1 (Первач)
GPIO 13  ────► Servo 2 (Головы)
GPIO 14  ────► Servo 3 (Тело)
GPIO 27  ────► Servo 4 (Хвосты)

5V       ────► VCC всех серво
GND      ────► GND всех серво
```

---

## 🍓 Raspberry Pi + Python

### Управление GPIO

```python
#!/usr/bin/env python3
"""
Raspberry Pi контроллер управления приемниками
Использует GPIO для управления реле/клапанами
"""

import paho.mqtt.client as mqtt
import json
import RPi.GPIO as GPIO
import time

# MQTT настройки
MQTT_BROKER = "192.168.1.100"
MQTT_PORT = 1883
MQTT_TOPIC = "distillery/areometer/fraction"

# GPIO пины (BCM нумерация)
PIN_FORESHOTS = 17
PIN_HEADS = 27
PIN_BODY = 22
PIN_TAILS = 23

# Состояние клапанов
current_fraction = None

def setup_gpio():
    """Инициализация GPIO"""
    GPIO.setmode(GPIO.BCM)
    GPIO.setwarnings(False)

    # Настраиваем все пины как выходы
    for pin in [PIN_FORESHOTS, PIN_HEADS, PIN_BODY, PIN_TAILS]:
        GPIO.setup(pin, GPIO.OUT)
        GPIO.output(pin, GPIO.LOW)  # Закрыты по умолчанию

    print("GPIO initialized")

def close_all_valves():
    """Закрыть все клапаны"""
    for pin in [PIN_FORESHOTS, PIN_HEADS, PIN_BODY, PIN_TAILS]:
        GPIO.output(pin, GPIO.LOW)
    print("All valves CLOSED")

def open_valve(pin, name):
    """Открыть конкретный клапан"""
    close_all_valves()
    time.sleep(0.5)  # Задержка между переключениями

    GPIO.output(pin, GPIO.HIGH)
    print(f"→ Opened {name} valve (GPIO {pin})")

def on_connect(client, userdata, flags, rc):
    """Callback при подключении к MQTT"""
    print(f"Connected to MQTT with code {rc}")
    client.subscribe(MQTT_TOPIC)
    print(f"Subscribed to {MQTT_TOPIC}")

def on_message(client, userdata, msg):
    """Callback при получении сообщения"""
    global current_fraction

    try:
        data = json.loads(msg.payload.decode())

        fraction = data.get('to')
        alcohol = data.get('alcohol', 0)
        volume = data.get('volume', 0)

        print(f"\n[{time.strftime('%H:%M:%S')}] Fraction change:")
        print(f"  From: {data.get('from')}")
        print(f"  To: {fraction}")
        print(f"  Alcohol: {alcohol}%")
        print(f"  Volume: {volume}ml")

        # Переключаем клапаны
        switch_to_fraction(fraction)
        current_fraction = fraction

    except Exception as e:
        print(f"Error processing message: {e}")

def switch_to_fraction(fraction):
    """Переключение на нужную фракцию"""
    if fraction == "foreshots":
        open_valve(PIN_FORESHOTS, "FORESHOTS")
    elif fraction == "heads":
        open_valve(PIN_HEADS, "HEADS")
    elif fraction == "body":
        open_valve(PIN_BODY, "BODY")
    elif fraction == "tails":
        open_valve(PIN_TAILS, "TAILS")
    elif fraction == "finished":
        close_all_valves()
        print("Distillation FINISHED")
    else:
        print(f"Unknown fraction: {fraction}")

def main():
    """Главная функция"""
    print("=" * 50)
    print("Smart Areometr - Receiver Controller")
    print("=" * 50)

    # Инициализация GPIO
    setup_gpio()
    close_all_valves()

    # Подключение к MQTT
    client = mqtt.Client(client_id="ReceiverController")
    client.on_connect = on_connect
    client.on_message = on_message

    try:
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
        print(f"Connecting to {MQTT_BROKER}:{MQTT_PORT}...")

        # Запуск цикла
        client.loop_forever()

    except KeyboardInterrupt:
        print("\n\nShutting down...")
        close_all_valves()
        GPIO.cleanup()
        client.disconnect()

if __name__ == "__main__":
    main()
```

### Установка на Raspberry Pi

```bash
# Установка зависимостей
sudo apt update
sudo apt install python3-pip python3-rpi.gpio
pip3 install paho-mqtt

# Копирование скрипта
sudo cp receiver_controller.py /usr/local/bin/

# Создание systemd service
sudo nano /etc/systemd/system/receiver-controller.service
```

**receiver-controller.service:**
```ini
[Unit]
Description=Smart Areometr Receiver Controller
After=network.target mosquitto.service

[Service]
Type=simple
User=pi
ExecStart=/usr/bin/python3 /usr/local/bin/receiver_controller.py
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

```bash
# Запуск сервиса
sudo systemctl enable receiver-controller
sudo systemctl start receiver-controller

# Проверка статуса
sudo systemctl status receiver-controller
```

---

## 🔗 Node-RED автоматизация

### Flow переключения приемников

```json
[
    {
        "id": "mqtt_fraction",
        "type": "mqtt in",
        "name": "Fraction Events",
        "topic": "distillery/areometer/fraction",
        "qos": "1",
        "broker": "mqtt_broker",
        "x": 150,
        "y": 100,
        "wires": [["parse_json"]]
    },
    {
        "id": "parse_json",
        "type": "json",
        "name": "Parse",
        "x": 350,
        "y": 100,
        "wires": [["switch_fraction", "log_event"]]
    },
    {
        "id": "switch_fraction",
        "type": "switch",
        "name": "Route by Fraction",
        "property": "payload.to",
        "rules": [
            {"t": "eq", "v": "foreshots"},
            {"t": "eq", "v": "heads"},
            {"t": "eq", "v": "body"},
            {"t": "eq", "v": "tails"},
            {"t": "eq", "v": "finished"}
        ],
        "x": 550,
        "y": 100,
        "wires": [
            ["open_foreshots"],
            ["open_heads"],
            ["open_body"],
            ["open_tails"],
            ["close_all"]
        ]
    },
    {
        "id": "open_foreshots",
        "type": "rpi-gpio out",
        "name": "Valve Foreshots",
        "pin": "17",
        "set": true,
        "x": 800,
        "y": 60,
        "wires": []
    },
    {
        "id": "open_heads",
        "type": "rpi-gpio out",
        "name": "Valve Heads",
        "pin": "27",
        "set": true,
        "x": 800,
        "y": 100,
        "wires": []
    },
    {
        "id": "open_body",
        "type": "rpi-gpio out",
        "name": "Valve Body",
        "pin": "22",
        "set": true,
        "x": 800,
        "y": 140,
        "wires": []
    },
    {
        "id": "open_tails",
        "type": "rpi-gpio out",
        "name": "Valve Tails",
        "pin": "23",
        "set": true,
        "x": 800,
        "y": 180,
        "wires": []
    },
    {
        "id": "close_all",
        "type": "function",
        "name": "Close All",
        "func": "return [\n  {topic: '17', payload: false},\n  {topic: '27', payload: false},\n  {topic: '22', payload: false},\n  {topic: '23', payload: false}\n];",
        "x": 800,
        "y": 220,
        "wires": [["gpio_out"]]
    },
    {
        "id": "log_event",
        "type": "debug",
        "name": "Log",
        "x": 550,
        "y": 200,
        "wires": []
    }
]
```

---

## 🏠 Home Assistant

### Автоматизация с уведомлениями

**configuration.yaml:**
```yaml
mqtt:
  sensor:
    - name: "Current Fraction"
      state_topic: "distillery/areometer/fraction"
      value_template: "{{ value_json.to }}"
      json_attributes_topic: "distillery/areometer/fraction"
      json_attributes_template: "{{ value_json | tojson }}"

automation:
  # Уведомление о смене фракции
  - alias: "Notify on Fraction Change"
    trigger:
      platform: state
      entity_id: sensor.current_fraction
    action:
      - service: notify.mobile_app
        data:
          title: "🍺 Смена фракции"
          message: >
            {{ trigger.to_state.state | upper }}
            ({{ trigger.to_state.attributes.alcohol }}%)

  # Переключение приемников через ESP32
  - alias: "Switch Receivers"
    trigger:
      platform: state
      entity_id: sensor.current_fraction
    action:
      - service: mqtt.publish
        data:
          topic: "distillery/control/receiver"
          payload: "{{ trigger.to_state.state }}"

  # Аварийная остановка при низкой крепости
  - alias: "Emergency Stop"
    trigger:
      platform: numeric_state
      entity_id: sensor.alcohol_percentage
      below: 20
    condition:
      condition: state
      entity_id: sensor.current_fraction
      state: "tails"
    action:
      - service: switch.turn_off
        entity_id: switch.heating_element
      - service: notify.telegram
        data:
          message: "⚠️ АВАРИЙНАЯ ОСТАНОВКА! Крепость <20%"
```

---

## 🏭 PLC/Промышленные контроллеры

### Modbus RTU → MQTT Gateway

Для интеграции с промышленными контроллерами используйте MQTT-Modbus gateway:

```python
#!/usr/bin/env python3
"""
MQTT → Modbus RTU Gateway
Преобразует события фракций в сигналы Modbus
"""

from pymodbus.client import ModbusSerialClient
import paho.mqtt.client as mqtt
import json

# Modbus настройки
MODBUS_PORT = '/dev/ttyUSB0'
MODBUS_BAUDRATE = 9600
MODBUS_SLAVE_ID = 1

# Адреса регистров (coils)
COIL_FORESHOTS = 0
COIL_HEADS = 1
COIL_BODY = 2
COIL_TAILS = 3

modbus_client = ModbusSerialClient(
    method='rtu',
    port=MODBUS_PORT,
    baudrate=MODBUS_BAUDRATE,
    timeout=1
)

def on_message(client, userdata, msg):
    data = json.loads(msg.payload.decode())
    fraction = data.get('to')

    # Сбрасываем все coils
    modbus_client.write_coils(0, [False, False, False, False], unit=MODBUS_SLAVE_ID)

    # Устанавливаем нужный coil
    if fraction == "foreshots":
        modbus_client.write_coil(COIL_FORESHOTS, True, unit=MODBUS_SLAVE_ID)
    elif fraction == "heads":
        modbus_client.write_coil(COIL_HEADS, True, unit=MODBUS_SLAVE_ID)
    elif fraction == "body":
        modbus_client.write_coil(COIL_BODY, True, unit=MODBUS_SLAVE_ID)
    elif fraction == "tails":
        modbus_client.write_coil(COIL_TAILS, True, unit=MODBUS_SLAVE_ID)

    print(f"Modbus: Set {fraction} coil")

mqtt_client = mqtt.Client()
mqtt_client.on_message = on_message
mqtt_client.connect("192.168.1.100", 1883)
mqtt_client.subscribe("distillery/areometer/fraction")
mqtt_client.loop_forever()
```

---

## 🧪 Тестирование

### Эмуляция событий фракций

```bash
# Первач
mosquitto_pub -h localhost -t "distillery/areometer/fraction" -m '{
  "event": "fraction_change",
  "from": "unknown",
  "to": "foreshots",
  "alcohol": 95.0,
  "volume": 30
}'

# Головы
mosquitto_pub -h localhost -t "distillery/areometer/fraction" -m '{
  "event": "fraction_change",
  "from": "foreshots",
  "to": "heads",
  "alcohol": 88.0,
  "volume": 150
}'

# Тело
mosquitto_pub -h localhost -t "distillery/areometer/fraction" -m '{
  "event": "fraction_change",
  "from": "heads",
  "to": "body",
  "alcohol": 82.0,
  "volume": 500
}'

# Хвосты
mosquitto_pub -h localhost -t "distillery/areometer/fraction" -m '{
  "event": "fraction_change",
  "from": "body",
  "to": "tails",
  "alcohol": 75.0,
  "volume": 1200
}'

# Завершено
mosquitto_pub -h localhost -t "distillery/areometer/fraction" -m '{
  "event": "fraction_change",
  "from": "tails",
  "to": "finished",
  "alcohol": 45.0,
  "volume": 1800
}'
```

---

## 🔒 Безопасность

### Рекомендации

1. **Дублирование сигналов:** Используйте подтверждения переключения
2. **Таймауты:** Автоматическое закрытие клапанов при потере связи
3. **Аварийная кнопка:** Физическая кнопка остановки
4. **Логирование:** Запись всех переключений
5. **Резервное питание:** UPS для критичных компонентов

### Проверка состояния

```python
# Отправка подтверждения обратно в MQTT
def send_confirmation(fraction, status):
    mqtt_client.publish(
        "distillery/control/status",
        json.dumps({
            "fraction": fraction,
            "valve_status": status,
            "timestamp": time.time()
        })
    )
```

---

## 📚 Дополнительные ресурсы

- [ESP32 Servo Library](https://github.com/madhephaestus/ESP32Servo)
- [RPi.GPIO Documentation](https://sourceforge.net/p/raspberry-gpio-python/wiki/Home/)
- [Node-RED GPIO Guide](https://nodered.org/docs/hardware/raspberrypi)
- [PyModbus Documentation](https://pymodbus.readthedocs.io/)

---

*Документ обновлен: 2025-12-15*
*Версия: 1.0*
