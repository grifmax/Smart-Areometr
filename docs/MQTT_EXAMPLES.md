# 📡 MQTT Integration Examples

Примеры интеграции Smart Areometr с различными системами через MQTT.

## 📋 Оглавление
- [Установка Mosquitto](#установка-mosquitto)
- [Home Assistant](#home-assistant)
- [Node-RED](#node-red)
- [Telegram Bot](#telegram-bot)
- [Python Scripts](#python-scripts)
- [Grafana + InfluxDB](#grafana--influxdb)

---

## 🦟 Установка Mosquitto

### Ubuntu/Debian
```bash
# Установка
sudo apt update
sudo apt install mosquitto mosquitto-clients

# Запуск
sudo systemctl start mosquitto
sudo systemctl enable mosquitto
```

### Docker
```bash
docker run -d \
  --name mosquitto \
  -p 1883:1883 \
  -p 9001:9001 \
  -v ./mosquitto.conf:/mosquitto/config/mosquitto.conf \
  eclipse-mosquitto
```

### mosquitto.conf (минимальная конфигурация)
```conf
listener 1883
allow_anonymous true
persistence true
persistence_location /mosquitto/data/

# Логирование
log_dest file /mosquitto/log/mosquitto.log
log_type all
```

### С авторизацией
```conf
listener 1883
allow_anonymous false
password_file /mosquitto/config/passwd

# Создание пользователя
mosquitto_passwd -c /mosquitto/config/passwd distillery
```

---

## 🏠 Home Assistant

### Вариант 1: MQTT Discovery (автоматический)

Smart Areometr автоматически публикует конфигурацию для Home Assistant.

**Требования:**
- MQTT интеграция включена в HA
- Discovery prefix = `homeassistant` (по умолчанию)

**Entities появятся автоматически:**
- `sensor.alcohol_percentage` - Крепость
- `sensor.temperature` - Температура
- `sensor.signal_stability` - Стабильность сигнала
- `sensor.distillation_fraction` - Текущая фракция

### Вариант 2: Ручная конфигурация

**configuration.yaml:**
```yaml
mqtt:
  broker: 192.168.1.100
  port: 1883
  username: !secret mqtt_user
  password: !secret mqtt_pass

  sensor:
    # Крепость
    - name: "Alcohol Percentage"
      unique_id: "areometer_alcohol"
      state_topic: "distillery/areometer/alcohol"
      unit_of_measurement: "%"
      icon: "mdi:bottle-wine"
      state_class: "measurement"
      device_class: "battery"  # Для процентов

    # Температура
    - name: "Distillation Temperature"
      unique_id: "areometer_temperature"
      state_topic: "distillery/areometer/temperature"
      unit_of_measurement: "°C"
      device_class: "temperature"
      state_class: "measurement"

    # Стабильность сигнала
    - name: "Signal Stability"
      unique_id: "areometer_stability"
      state_topic: "distillery/areometer/stability"
      unit_of_measurement: "%"
      icon: "mdi:signal"
      state_class: "measurement"

    # Фракция
    - name: "Current Fraction"
      unique_id: "areometer_fraction"
      state_topic: "distillery/areometer/fraction"
      icon: "mdi:beaker"

  binary_sensor:
    # Доступность
    - name: "Areometer Online"
      unique_id: "areometer_online"
      state_topic: "distillery/areometer/availability"
      payload_on: "online"
      payload_off: "offline"
      device_class: "connectivity"
```

### Автоматизации

**Уведомление при смене фракции:**
```yaml
automation:
  - alias: "Notify on fraction change"
    trigger:
      platform: state
      entity_id: sensor.current_fraction
    action:
      - service: notify.telegram
        data:
          message: >
            🍺 Смена фракции: {{ trigger.to_state.state }}
            Крепость: {{ states('sensor.alcohol_percentage') }}%
            Температура: {{ states('sensor.distillation_temperature') }}°C

  - alias: "Alert on low alcohol"
    trigger:
      platform: numeric_state
      entity_id: sensor.alcohol_percentage
      below: 40
    action:
      - service: notify.mobile_app
        data:
          title: "⚠️ Низкая крепость"
          message: "Крепость упала ниже 40% - переключите приемник!"
```

**Lovelace Dashboard:**
```yaml
type: vertical-stack
cards:
  # Текущие показания
  - type: entities
    title: Smart Areometr
    entities:
      - entity: sensor.alcohol_percentage
        name: Крепость
        icon: mdi:bottle-wine
      - entity: sensor.distillation_temperature
        name: Температура
        icon: mdi:thermometer
      - entity: sensor.signal_stability
        name: Стабильность
        icon: mdi:signal
      - entity: sensor.current_fraction
        name: Фракция
        icon: mdi:beaker
      - entity: binary_sensor.areometer_online
        name: Статус
        icon: mdi:wifi

  # График крепости
  - type: history-graph
    title: История крепости
    hours_to_show: 4
    entities:
      - entity: sensor.alcohol_percentage
        name: Крепость

  # График температуры
  - type: history-graph
    title: Температура
    hours_to_show: 4
    entities:
      - entity: sensor.distillation_temperature
        name: Температура
```

---

## 🔗 Node-RED

### Базовый flow (мониторинг)

```json
[
  {
    "id": "mqtt_in",
    "type": "mqtt in",
    "z": "flow_id",
    "name": "Areometer State",
    "topic": "distillery/areometer/state",
    "qos": "0",
    "broker": "mqtt_broker",
    "x": 150,
    "y": 100,
    "wires": [["parse_json"]]
  },
  {
    "id": "parse_json",
    "type": "json",
    "z": "flow_id",
    "name": "Parse JSON",
    "x": 350,
    "y": 100,
    "wires": [["display_alcohol", "check_threshold"]]
  },
  {
    "id": "display_alcohol",
    "type": "debug",
    "z": "flow_id",
    "name": "Show Alcohol %",
    "active": true,
    "x": 550,
    "y": 80,
    "wires": []
  },
  {
    "id": "check_threshold",
    "type": "switch",
    "z": "flow_id",
    "name": "Check if < 40%",
    "property": "payload.alcohol",
    "rules": [
      {"t": "lt", "v": "40"}
    ],
    "x": 550,
    "y": 120,
    "wires": [["send_alert"]]
  },
  {
    "id": "send_alert",
    "type": "telegram sender",
    "z": "flow_id",
    "name": "Telegram Alert",
    "bot": "telegram_bot",
    "chatId": "",
    "message": "⚠️ Крепость ниже 40%!",
    "x": 750,
    "y": 120,
    "wires": [[]]
  }
]
```

### Логирование в файл

```javascript
// Function node
var alcohol = msg.payload.alcohol;
var temp = msg.payload.temperature;
var timestamp = new Date().toISOString();

var logEntry = {
    timestamp: timestamp,
    alcohol: alcohol,
    temperature: temp,
    fraction: msg.payload.fraction
};

// Добавляем в буфер
flow.set("log_buffer", flow.get("log_buffer") || []);
var buffer = flow.get("log_buffer");
buffer.push(logEntry);

// Если накопилось 10 записей - сохраняем
if (buffer.length >= 10) {
    msg.payload = buffer;
    flow.set("log_buffer", []);
    return msg;
}

return null;
```

---

## 🤖 Telegram Bot

### Python + python-telegram-bot

```python
import paho.mqtt.client as mqtt
from telegram import Bot
from telegram.ext import Updater, CommandHandler
import asyncio

# Конфигурация
MQTT_BROKER = "192.168.1.100"
MQTT_PORT = 1883
TELEGRAM_TOKEN = "YOUR_BOT_TOKEN"
CHAT_ID = "YOUR_CHAT_ID"

bot = Bot(token=TELEGRAM_TOKEN)
current_data = {}

# MQTT Callbacks
def on_connect(client, userdata, flags, rc):
    print(f"Connected to MQTT with code {rc}")
    client.subscribe("distillery/areometer/#")

def on_message(client, userdata, msg):
    topic = msg.topic
    payload = msg.payload.decode()

    # Сохраняем данные
    if topic.endswith("/alcohol"):
        current_data['alcohol'] = float(payload)
    elif topic.endswith("/temperature"):
        current_data['temperature'] = float(payload)
    elif topic.endswith("/fraction"):
        old_fraction = current_data.get('fraction')
        current_data['fraction'] = payload

        # Уведомление при смене фракции
        if old_fraction and old_fraction != payload:
            send_telegram(f"🍺 Смена фракции: {payload}")

# Telegram уведомление
def send_telegram(message):
    asyncio.run(bot.send_message(chat_id=CHAT_ID, text=message))

# Команда /status
def status_command(update, context):
    message = f"""
📊 Статус ареометра:
Крепость: {current_data.get('alcohol', 'N/A')}%
Температура: {current_data.get('temperature', 'N/A')}°C
Фракция: {current_data.get('fraction', 'N/A')}
    """
    update.message.reply_text(message)

# Запуск
mqtt_client = mqtt.Client()
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message
mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
mqtt_client.loop_start()

updater = Updater(TELEGRAM_TOKEN, use_context=True)
updater.dispatcher.add_handler(CommandHandler('status', status_command))
updater.start_polling()
updater.idle()
```

---

## 🐍 Python Scripts

### Простой мониторинг

```python
import paho.mqtt.client as mqtt
import json
from datetime import datetime

def on_connect(client, userdata, flags, rc):
    print(f"Connected with result code {rc}")
    client.subscribe("distillery/areometer/state")

def on_message(client, userdata, msg):
    data = json.loads(msg.payload)
    timestamp = datetime.now().strftime("%H:%M:%S")

    print(f"[{timestamp}] "
          f"Alcohol: {data['alcohol']}% | "
          f"Temp: {data['temperature']}°C | "
          f"Stability: {data['stability']}%")

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect("192.168.1.100", 1883, 60)
client.loop_forever()
```

### Логирование в CSV

```python
import paho.mqtt.client as mqtt
import json
import csv
from datetime import datetime

CSV_FILE = "distillation_log.csv"

def on_connect(client, userdata, flags, rc):
    client.subscribe("distillery/areometer/state")
    print("Logging to", CSV_FILE)

def on_message(client, userdata, msg):
    data = json.loads(msg.payload)

    with open(CSV_FILE, 'a', newline='') as f:
        writer = csv.writer(f)

        # Если файл пустой - пишем заголовок
        if f.tell() == 0:
            writer.writerow(['Timestamp', 'Alcohol %', 'Temperature °C', 'Stability %', 'Fraction'])

        writer.writerow([
            datetime.now().isoformat(),
            data.get('alcohol', ''),
            data.get('temperature', ''),
            data.get('stability', ''),
            data.get('fraction', '')
        ])

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.connect("192.168.1.100", 1883, 60)
client.loop_forever()
```

---

## 📊 Grafana + InfluxDB

### Telegraf конфигурация

**telegraf.conf:**
```toml
[[inputs.mqtt_consumer]]
  servers = ["tcp://192.168.1.100:1883"]
  topics = ["distillery/areometer/state"]
  data_format = "json"
  json_name_key = "measurement"
  tag_keys = ["fraction"]

[[outputs.influxdb_v2]]
  urls = ["http://localhost:8086"]
  token = "YOUR_INFLUX_TOKEN"
  organization = "distillery"
  bucket = "areometer"
```

### InfluxDB запросы

```flux
// Крепость за последние 4 часа
from(bucket: "areometer")
  |> range(start: -4h)
  |> filter(fn: (r) => r["_measurement"] == "state")
  |> filter(fn: (r) => r["_field"] == "alcohol")

// Средняя крепость по фракциям
from(bucket: "areometer")
  |> range(start: -24h)
  |> filter(fn: (r) => r["_field"] == "alcohol")
  |> group(columns: ["fraction"])
  |> mean()
```

### Grafana Dashboard

**Panel 1: Time Series (крепость)**
```
Query: SELECT mean("alcohol") FROM "state" WHERE $timeFilter GROUP BY time(10s)
```

**Panel 2: Gauge (текущая крепость)**
```
Query: SELECT last("alcohol") FROM "state"
Thresholds: 0-40 (red), 40-85 (green), 85-100 (yellow)
```

**Panel 3: Stat (фракция)**
```
Query: SELECT last("fraction") FROM "state"
Value mappings: heads=Головы, body=Тело, tails=Хвосты
```

---

## 🧪 Тестирование

### Публикация тестовых данных

```bash
# Простые данные
mosquitto_pub -h localhost -t "distillery/areometer/alcohol" -m "42.5"
mosquitto_pub -h localhost -t "distillery/areometer/temperature" -m "20.3"

# Полное состояние (JSON)
mosquitto_pub -h localhost -t "distillery/areometer/state" -m '{
  "alcohol": 42.5,
  "temperature": 20.3,
  "stability": 85,
  "fraction": "body",
  "timestamp": 1234567890
}'
```

### Отправка команд

```bash
# Перезагрузка
mosquitto_pub -h localhost -t "distillery/areometer/command" -m "restart"

# Калибровка
mosquitto_pub -h localhost -t "distillery/areometer/calibrate" -m '{
  "alcohol": 40.0,
  "temperature": 20.0
}'
```

### Подписка на все топики

```bash
# Все сообщения
mosquitto_sub -h localhost -t "distillery/areometer/#" -v

# Только данные (без команд)
mosquitto_sub -h localhost -t "distillery/areometer/+" -v
```

---

## 🔒 Безопасность

### SSL/TLS (Mosquitto)

**mosquitto.conf:**
```conf
listener 8883
cafile /mosquitto/config/ca.crt
certfile /mosquitto/config/server.crt
keyfile /mosquitto/config/server.key
require_certificate false
```

### Генерация сертификатов

```bash
# CA сертификат
openssl req -new -x509 -days 3650 -extensions v3_ca \
  -keyout ca.key -out ca.crt

# Сертификат сервера
openssl genrsa -out server.key 2048
openssl req -new -out server.csr -key server.key
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key \
  -CAcreateserial -out server.crt -days 3650
```

### ACL (контроль доступа)

**acl.conf:**
```
# Читать всем
topic read distillery/areometer/#

# Писать только устройству
user smart-areometr
topic write distillery/areometer/#

# Команды только для admin
user admin
topic readwrite distillery/areometer/command
topic readwrite distillery/areometer/calibrate
```

---

## 📚 Дополнительные ресурсы

- [MQTT.org](https://mqtt.org/) - Официальная спецификация
- [Mosquitto Documentation](https://mosquitto.org/documentation/)
- [Home Assistant MQTT](https://www.home-assistant.io/integrations/mqtt/)
- [Node-RED MQTT](https://cookbook.nodered.org/mqtt/)
- [Paho Python Client](https://www.eclipse.org/paho/index.php?page=clients/python/index.php)

---

*Документ обновлен: 2025-12-15*
*Версия: 1.0*
