# Фронтенд Smart Areometr

Современный веб-интерфейс для управления и мониторинга электронного ареометра.

## Структура

```
data/
├── index.html              # Главная страница
├── calibration.html        # Страница калибровки
├── settings.html           # Страница настроек
├── mqtt.html               # Настройки MQTT
├── fractions.html          # Управление фракциями
├── distillation.html       # Сессии дистилляции
├── logs.html               # История измерений
├── css/
│   ├── style.css           # Основные стили
│   ├── calibration.css     # Стили калибровки
│   ├── dilution-calculator.css  # Стили калькулятора разбавления
│   └── logs.css            # Стили логов
└── js/
    ├── app.js              # Главная страница JS
    ├── calibration.js      # Калибровка JS
    ├── settings.js         # Настройки JS
    ├── mqtt.js             # MQTT настройки JS
    ├── fractions.js        # Фракции JS
    ├── distillation.js     # Сессии дистилляции JS
    ├── logs.js             # Логи JS
    ├── theme.js            # Управление темами
    ├── dilution-calculator.js      # Калькулятор разбавления
    └── dilution-calculator-ui.js  # UI калькулятора
```

## Возможности

### Главная страница (/)
- Real-time мониторинг крепости и температуры
- Живые графики (Chart.js)
- Автообновление каждые 2 секунды
- Быстрые действия (измерение, калибровка)
- Экспорт данных в JSON

### Калибровка (/calibration.html)
- Пошаговый мастер калибровки
- Визуализация процесса
- Отображение сырых значений датчика
- Расширенные настройки температурной компенсации

### Настройки (/settings.html)
- Настройка Wi-Fi подключения
- Параметры измерения (выборки, задержки)
- Температурная компенсация
- Системная информация
- OTA обновления
- Выбор темы оформления (светлая, темная, голубая, пурпурная, киберпанк, оранжевая)

### MQTT (/mqtt.html)
- Настройка подключения к MQTT broker
- Статус подключения в реальном времени
- Список топиков (скрывающийся раздел)
- Примеры использования (скрывающийся раздел)
- Тест подключения

### Фракции (/fractions.html)
- Настройка порогов для определения фракций
- Режимы работы (mash/monitoring)
- Визуализация текущей фракции
- Калькулятор разбавления

### Сессии дистилляции (/distillation.html)
- Создание и управление сессиями
- Отслеживание объема браги и собранного дистиллята
- Статистика по фракциям
- История сессий

### Логи (/logs.html)
- Таблица всех измерений
- Статистика (среднее, мин/макс)
- Фильтрация и сортировка
- Экспорт в CSV и JSON

## Загрузка файлов в ESP32

### Метод 1: PlatformIO (рекомендуется)

1. Убедитесь, что папка `data/` находится в корне проекта
2. Установите инструмент для загрузки файловой системы:
   ```bash
   pio pkg install --tool "platformio/tool-mkspiffs"
   ```

3. Соберите файловую систему:
   ```bash
   pio run --target buildfs
   ```

4. Загрузите в ESP32:
   ```bash
   pio run --target uploadfs
   ```

### Метод 2: Arduino IDE

1. Установите плагин ESP32 Sketch Data Upload:
   - Скачайте с https://github.com/me-no-dev/arduino-esp32fs-plugin
   - Распакуйте в `Arduino/tools/`

2. Поместите папку `data/` в папку вашего скетча

3. Выберите в меню: `Tools -> ESP32 Sketch Data Upload`

### Метод 3: Веб-загрузка (в разработке)

Планируется добавить веб-интерфейс для загрузки файлов без пересборки.

## Разработка

### Локальная разработка

Для локальной разработки можно использовать любой веб-сервер:

```bash
# Python
cd data
python -m http.server 8000

# Node.js
npx http-server data -p 8000
```

Откройте http://localhost:8000 в браузере.

**Важно:** Для работы API нужно настроить прокси к ESP32 или использовать CORS.

### Минификация (опционально)

Для уменьшения размера файлов можно использовать минификацию:

```bash
# CSS
npx csso style.css --output style.min.css

# JavaScript
npx terser app.js --compress --mangle --output app.min.js

# HTML
npx html-minifier index.html --output index.min.html \
  --collapse-whitespace \
  --remove-comments \
  --minify-css \
  --minify-js
```

## Технологии

- **Чистый JavaScript** (без фреймворков для минимального размера)
- **Chart.js 4.4.0** для графиков (загружается с CDN)
- **CSS Grid & Flexbox** для responsive layout
- **Fetch API** для работы с REST API
- **LocalStorage** (планируется) для кеширования

## API Endpoints

Фронтенд использует следующие API endpoints:

### GET /api/measurement
Получить текущие измерения
```json
{
  "alcohol": 45.2,
  "temperature": 22.5,
  "calibrated": true,
  "timestamp": 12345678
}
```

### GET /api/status
Системная информация
```json
{
  "firmware": "2.1.4",
  "wifi_mode": "AP",
  "ssid": "Areometr_AP",
  "ip": "192.168.4.1",
  "calibrated": true
}
```

### GET /api/logs
История измерений
```json
{
  "measurements": [
    {
      "timestamp": 12345678,
      "alcohol": 45.2,
      "temperature": 22.5,
      "compensated": true
    }
  ]
}
```

### POST /api/calibrate/water
Запустить калибровку на воде

### POST /api/calibrate/alcohol
Запустить калибровку на спирте

### GET /api/calibration
Получить калибровочные данные

### DELETE /api/logs
Очистить историю измерений

### GET /api/mqtt/status
Получить статус MQTT подключения

### GET /api/mqtt/config
Получить конфигурацию MQTT

### POST /api/mqtt/config
Сохранить конфигурацию MQTT

### POST /api/mqtt/test
Тест MQTT подключения

### GET /api/fractions/thresholds
Получить пороги фракций

### POST /api/fractions/thresholds
Установить пороги фракций

### GET /api/session/active
Получить активную сессию дистилляции

### POST /api/session/start
Начать сессию дистилляции

### POST /api/session/stop
Остановить сессию дистилляции

## Размер файлов

| Файл | Размер | Минифицированный |
|------|--------|------------------|
| index.html | ~5 KB | ~3 KB |
| calibration.html | ~7 KB | ~4 KB |
| settings.html | ~6 KB | ~4 KB |
| logs.html | ~5 KB | ~3 KB |
| mqtt.html | ~6 KB | ~4 KB |
| fractions.html | ~7 KB | ~4 KB |
| distillation.html | ~8 KB | ~5 KB |
| style.css | ~8 KB | ~5 KB |
| calibration.css | ~3 KB | ~2 KB |
| logs.css | ~3 KB | ~2 KB |
| dilution-calculator.css | ~2 KB | ~1 KB |
| app.js | ~6 KB | ~4 KB |
| calibration.js | ~4 KB | ~3 KB |
| settings.js | ~5 KB | ~3 KB |
| mqtt.js | ~4 KB | ~3 KB |
| fractions.js | ~5 KB | ~3 KB |
| distillation.js | ~6 KB | ~4 KB |
| logs.js | ~4 KB | ~3 KB |
| theme.js | ~3 KB | ~2 KB |
| dilution-calculator.js | ~4 KB | ~3 KB |
| dilution-calculator-ui.js | ~3 KB | ~2 KB |
| **Итого** | **~96 KB** | **~60 KB** |

ESP32-C3 имеет 4MB Flash, LittleFS займет ~300KB включая файловую систему.

## Адаптивность

Веб-интерфейс полностью адаптивен и работает на:
- Desktop (1200px+)
- Tablet (768px - 1200px)
- Mobile (< 768px)

## Браузеры

Поддерживаются все современные браузеры:
- Chrome/Edge 90+
- Firefox 88+
- Safari 14+
- Opera 76+

## Будущие улучшения

- [x] Темная тема (реализовано 6 тем)
- [ ] PWA поддержка (работа офлайн)
- [ ] WebSocket для real-time обновлений
- [ ] Многоязычность (EN/RU)
- [x] Графики с историей за период (реализовано)
- [ ] Экспорт в PDF
- [ ] Уведомления браузера
- [ ] Голосовые команды

## Лицензия

MIT License - см. файл LICENSE в корне проекта
