# API Документация

## REST API

Веб-сервер предоставляет RESTful API для доступа к данным и управления устройством.

**Base URL:** `http://<device-ip>/api`

## Endpoints

### 1. Получить текущее измерение

Возвращает последнее измерение процента алкоголя и температуры.

**Endpoint:** `GET /api/measurement`

**Response:**
```json
{
  "alcohol": 45.2,
  "temperature": 22.5,
  "calibrated": true,
  "timestamp": 12345678
}
```

**Поля:**
- `alcohol` (float): Процент алкоголя (0-100)
- `temperature` (float): Температура в °C
- `calibrated` (boolean): Откалиброван ли датчик
- `timestamp` (long): Метка времени (миллисекунды с запуска)

**Пример использования:**
```bash
curl http://192.168.4.1/api/measurement
```

```python
import requests
response = requests.get('http://192.168.4.1/api/measurement')
data = response.json()
print(f"Alcohol: {data['alcohol']}%")
```

---

### 2. Получить статус устройства

Возвращает информацию о состоянии устройства.

**Endpoint:** `GET /api/status`

**Response:**
```json
{
  "firmware": "1.0.0",
  "wifi_mode": "AP",
  "ssid": "Areometr_AP",
  "ip": "192.168.4.1",
  "calibrated": true
}
```

**Поля:**
- `firmware` (string): Версия прошивки
- `wifi_mode` (string): Режим Wi-Fi ("AP" или "Client")
- `ssid` (string): SSID сети
- `ip` (string): IP адрес устройства
- `calibrated` (boolean): Статус калибровки

**Пример:**
```bash
curl http://192.168.4.1/api/status
```

---

### 3. Калибровка на воде

Запускает процесс калибровки на чистой воде (0% алкоголя).

**Endpoint:** `POST /api/calibrate/water`

**Request:** Без параметров

**Response:**
```json
{
  "status": "calibration_started",
  "step": "water"
}
```

**Пример:**
```bash
curl -X POST http://192.168.4.1/api/calibrate/water
```

**Примечание:** Датчик должен быть погружен в чистую воду перед вызовом.

---

### 4. Калибровка на спирте

Запускает процесс калибровки на чистом спирте (100% алкоголя).

**Endpoint:** `POST /api/calibrate/alcohol`

**Request:** Без параметров

**Response:**
```json
{
  "status": "calibration_started",
  "step": "alcohol"
}
```

**Пример:**
```bash
curl -X POST http://192.168.4.1/api/calibrate/alcohol
```

**Примечание:** Датчик должен быть погружен в чистый спирт перед вызовом.

---

### 5. MQTT: Получить статус подключения

Возвращает информацию о состоянии MQTT подключения.

**Endpoint:** `GET /api/mqtt/status`

**Response:**
```json
{
  "connected": true,
  "enabled": true,
  "server": "192.168.1.100",
  "port": 1883,
  "base_topic": "distillery/areometer",
  "published_count": 1234
}
```

**Поля:**
- `connected` (boolean): Подключен ли к MQTT broker
- `enabled` (boolean): Включен ли MQTT
- `server` (string): Адрес MQTT broker
- `port` (int): Порт MQTT broker
- `base_topic` (string): Базовый топик
- `published_count` (int): Количество опубликованных сообщений

**Пример:**
```bash
curl http://192.168.4.1/api/mqtt/status
```

---

### 6. MQTT: Получить конфигурацию

Возвращает текущую конфигурацию MQTT.

**Endpoint:** `GET /api/mqtt/config`

**Response:**
```json
{
  "enabled": true,
  "server": "192.168.1.100",
  "port": 1883,
  "username": "user",
  "client_id": "smart-areometr",
  "base_topic": "distillery/areometer",
  "publish_interval": 5,
  "ha_discovery": true
}
```

**Примечание:** Пароль не возвращается в целях безопасности.

**Пример:**
```bash
curl http://192.168.4.1/api/mqtt/config
```

---

### 7. MQTT: Сохранить конфигурацию

Сохраняет новую конфигурацию MQTT.

**Endpoint:** `POST /api/mqtt/config`

**Request Body:**
```json
{
  "enabled": true,
  "server": "192.168.1.100",
  "port": 1883,
  "username": "user",
  "password": "pass",
  "client_id": "smart-areometr",
  "base_topic": "distillery/areometer",
  "publish_interval": 5,
  "ha_discovery": true
}
```

**Response:**
```json
{
  "status": "success"
}
```

**Пример:**
```bash
curl -X POST http://192.168.4.1/api/mqtt/config \
  -H "Content-Type: application/json" \
  -d '{"enabled":true,"server":"192.168.1.100","port":1883}'
```

**Примечание:** После сохранения требуется перезагрузка устройства для применения настроек.

---

### 8. MQTT: Тест подключения

Проверяет возможность подключения к MQTT broker.

**Endpoint:** `POST /api/mqtt/test`

**Request:** Без параметров

**Response:**
```json
{
  "success": true,
  "connected": true
}
```

или при ошибке:
```json
{
  "success": false,
  "connected": false,
  "error": "Connection timeout"
}
```

**Пример:**
```bash
curl -X POST http://192.168.4.1/api/mqtt/test
```

---

### 9. Фракции: Получить пороги

Возвращает пороги для определения фракций.

**Endpoint:** `GET /api/fractions/thresholds`

**Response:**
```json
{
  "heads_threshold": 75.0,
  "body_threshold": 40.0,
  "mode": "mash"
}
```

**Пример:**
```bash
curl http://192.168.4.1/api/fractions/thresholds
```

---

### 10. Фракции: Установить пороги

Устанавливает пороги для определения фракций.

**Endpoint:** `POST /api/fractions/thresholds`

**Request Body:**
```json
{
  "heads_threshold": 75.0,
  "body_threshold": 40.0
}
```

**Response:**
```json
{
  "status": "success"
}
```

**Пример:**
```bash
curl -X POST http://192.168.4.1/api/fractions/thresholds \
  -H "Content-Type: application/json" \
  -d '{"heads_threshold":75.0,"body_threshold":40.0}'
```

---

### 11. Сессия: Начать дистилляцию

Начинает новую сессию дистилляции.

**Endpoint:** `POST /api/session/start`

**Request Body:**
```json
{
  "name": "Дистилляция #1",
  "mash_volume": 20.0
}
```

**Response:**
```json
{
  "status": "success"
}
```

**Пример:**
```bash
curl -X POST http://192.168.4.1/api/session/start \
  -H "Content-Type: application/json" \
  -d '{"name":"Дистилляция #1","mash_volume":20.0}'
```

---

### 12. Сессия: Остановить дистилляцию

Останавливает текущую сессию дистилляции.

**Endpoint:** `POST /api/session/stop`

**Request:** Без параметров

**Response:**
```json
{
  "status": "success"
}
```

**Пример:**
```bash
curl -X POST http://192.168.4.1/api/session/stop
```

---

### 13. Сессия: Получить активную сессию

Возвращает информацию об активной сессии.

**Endpoint:** `GET /api/session/active`

**Response:**
```json
{
  "active": true,
  "name": "Дистилляция #1",
  "start_time": 1234567890,
  "mash_volume": 20.0,
  "collected_volume": 5.2,
  "current_fraction": "body"
}
```

**Пример:**
```bash
curl http://192.168.4.1/api/session/active
```

---

## Планируемые endpoints (TODO)

### Получить историю измерений

**Endpoint:** `GET /api/logs` (реализовано)

**Response:**
```json
{
  "count": 50,
  "measurements": [
    {
      "timestamp": 12345678,
      "alcohol": 45.2,
      "temperature": 22.5,
      "compensated": true,
      "fraction": "body"
    },
    ...
  ]
}
```

**Пример:**
```bash
curl http://192.168.4.1/api/logs
```

---

### 20. Приемники: Получить статус

Возвращает статус всех приемников.

**Endpoint:** `GET /api/receivers/status`

**Response:**
```json
{
  "enabled": true,
  "auto_switch": true,
  "active_receiver": 0,
  "overflow_action": "switch_next",
  "receivers": [
    {
      "id": 0,
      "name": "Головы",
      "active": true,
      "overflowing": false,
      "current_volume": 150.5,
      "max_volume": 1000,
      "fraction": "HEADS",
      "gpio_pin": 7
    }
  ]
}
```

---

### 21. Приемники: Переключить

Переключает на указанный приемник.

**Endpoint:** `POST /api/receivers/switch`

**Request Body:**
```json
{
  "receiver_id": 0
}
```

---

### 22. Датчики: Получить статус

Возвращает статус всех датчиков (мультисенсорный режим).

**Endpoint:** `GET /api/sensors/status`

**Response:**
```json
{
  "sensor_count": 2,
  "max_sensors": 4,
  "average_alcohol": 45.2,
  "average_temperature": 22.5,
  "anomalies_detected": false,
  "sensors": [
    {
      "id": 0,
      "name": "Основной датчик",
      "alcohol": 45.0,
      "temperature": 22.0,
      "stability": 95,
      "raw_value": 12345,
      "active": true,
      "calibrated": true,
      "last_update": 1234567890
    }
  ]
}
```

---

### 23. Датчики: Включить/выключить

Включает или выключает датчик.

**Endpoint:** `POST /api/sensors/enable`

**Request Body:**
```json
{
  "sensor_id": 0,
  "enabled": true
}
```

---

### Очистить логи

**Endpoint:** `DELETE /api/logs` (реализовано)

**Query Parameters:**
- `limit` (int): Максимальное количество записей (по умолчанию: 100)
- `offset` (int): Смещение для пагинации

**Response:**
```json
{
  "count": 50,
  "measurements": [
    {
      "timestamp": 12345678,
      "alcohol": 45.2,
      "temperature": 22.5,
      "compensated": true
    },
    ...
  ]
}
```

---

### Очистить логи

**Endpoint:** `DELETE /api/logs` (реализовано)

**Response:**
```json
{
  "status": "success",
  "message": "All logs cleared"
}
```

**Пример:**
```bash
curl -X DELETE http://192.168.4.1/api/logs
```

---

### Настройки Wi-Fi

**Endpoint:** `POST /api/wifi/config` (реализовано)

**Request Body:**
```json
{
  "ssid": "MyNetwork",
  "password": "mypassword"
}
```

**Response:**
```json
{
  "status": "success",
  "message": "Wi-Fi configuration updated. Rebooting..."
}
```

---

### Получить калибровочные данные

**Endpoint:** `GET /api/calibration` (реализовано)

**Response:**
```json
{
  "water_value": 150.5,
  "alcohol_value": 85.2,
  "temperature_reference": 20.0,
  "temperature_coefficient": 0.4
}
```

**Пример:**
```bash
curl http://192.168.4.1/api/calibration
```

---

### Установить калибровочные данные

**Endpoint:** `POST /api/calibration` (реализовано)

**Request Body:**
```json
{
  "water_value": 150.5,
  "alcohol_value": 85.2
}
```

**Response:**
```json
{
  "status": "success",
  "message": "Calibration data updated"
}
```

**Пример:**
```bash
curl -X POST http://192.168.4.1/api/calibration \
  -H "Content-Type: application/json" \
  -d '{"water_value":150.5,"alcohol_value":85.2}'
```

---

## WebSocket API (планируется)

Для получения данных в реальном времени.

**Endpoint:** `ws://<device-ip>/ws`

**События:**
- `measurement`: Новое измерение
- `calibration`: Обновление статуса калибровки
- `error`: Ошибка устройства

**Пример сообщения:**
```json
{
  "event": "measurement",
  "data": {
    "alcohol": 45.2,
    "temperature": 22.5,
    "timestamp": 12345678
  }
}
```

---

## Коды ошибок

| Код | Описание |
|-----|----------|
| 200 | OK - Запрос выполнен успешно |
| 400 | Bad Request - Неверные параметры |
| 404 | Not Found - Эндпоинт не найден |
| 500 | Internal Server Error - Внутренняя ошибка |
| 503 | Service Unavailable - Датчик не откалиброван |

---

## Примеры интеграции

### Python скрипт для мониторинга

```python
import requests
import time

DEVICE_IP = "192.168.4.1"
API_URL = f"http://{DEVICE_IP}/api"

def get_measurement():
    try:
        response = requests.get(f"{API_URL}/measurement")
        return response.json()
    except Exception as e:
        print(f"Error: {e}")
        return None

while True:
    data = get_measurement()
    if data:
        print(f"Alcohol: {data['alcohol']:.1f}% | Temp: {data['temperature']:.1f}°C")
    time.sleep(5)
```

### JavaScript (Node.js)

```javascript
const axios = require('axios');

const DEVICE_IP = '192.168.4.1';
const API_URL = `http://${DEVICE_IP}/api`;

async function getMeasurement() {
  try {
    const response = await axios.get(`${API_URL}/measurement`);
    console.log(`Alcohol: ${response.data.alcohol}%`);
    console.log(`Temperature: ${response.data.temperature}°C`);
  } catch (error) {
    console.error('Error:', error.message);
  }
}

setInterval(getMeasurement, 5000);
```

### Home Assistant интеграция

```yaml
sensor:
  - platform: rest
    name: Areometr Alcohol
    resource: http://192.168.4.1/api/measurement
    value_template: '{{ value_json.alcohol }}'
    unit_of_measurement: '%'
    scan_interval: 30

  - platform: rest
    name: Areometr Temperature
    resource: http://192.168.4.1/api/measurement
    value_template: '{{ value_json.temperature }}'
    unit_of_measurement: '°C'
    scan_interval: 30
```

---

## Rate Limiting

В текущей версии нет ограничений на количество запросов, но рекомендуется:
- Не более 1 запроса в секунду для `/api/measurement`
- Не более 1 запроса в минуту для калибровки

---

## CORS

CORS заголовки включены по умолчанию для всех endpoints, что позволяет делать запросы из браузера с других доменов.

---

## Аутентификация

В текущей версии аутентификация не реализована. Планируется добавить:
- Basic Auth для API endpoints
- API ключи для программного доступа
- JWT токены для веб-интерфейса
