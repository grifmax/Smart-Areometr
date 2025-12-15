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

## Планируемые endpoints (TODO)

### Получить историю измерений

**Endpoint:** `GET /api/logs`

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

**Endpoint:** `DELETE /api/logs`

**Response:**
```json
{
  "status": "success",
  "message": "All logs cleared"
}
```

---

### Настройки Wi-Fi

**Endpoint:** `POST /api/wifi/config`

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

**Endpoint:** `GET /api/calibration`

**Response:**
```json
{
  "water_value": 150.5,
  "alcohol_value": 85.2,
  "temperature_reference": 20.0,
  "temperature_coefficient": 0.4
}
```

---

### Установить калибровочные данные

**Endpoint:** `POST /api/calibration`

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
