# Фронтенд Smart Areometr

Современный веб-интерфейс для управления и мониторинга электронного ареометра.

## Структура

```
data/
├── index.html              # Главная страница
├── calibration.html        # Страница калибровки
├── settings.html           # Страница настроек
├── logs.html               # История измерений
├── css/
│   ├── style.css           # Основные стили
│   ├── calibration.css     # Стили калибровки
│   └── logs.css            # Стили логов
└── js/
    ├── app.js              # Главная страница JS
    ├── calibration.js      # Калибровка JS
    ├── settings.js         # Настройки JS
    └── logs.js             # Логи JS
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
  "firmware": "1.0.0",
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

## Размер файлов

| Файл | Размер | Минифицированный |
|------|--------|------------------|
| index.html | ~5 KB | ~3 KB |
| calibration.html | ~7 KB | ~4 KB |
| settings.html | ~6 KB | ~4 KB |
| logs.html | ~5 KB | ~3 KB |
| style.css | ~8 KB | ~5 KB |
| calibration.css | ~3 KB | ~2 KB |
| logs.css | ~3 KB | ~2 KB |
| app.js | ~6 KB | ~4 KB |
| calibration.js | ~4 KB | ~3 KB |
| settings.js | ~5 KB | ~3 KB |
| logs.js | ~4 KB | ~3 KB |
| **Итого** | **~56 KB** | **~36 KB** |

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

- [ ] Темная тема
- [ ] PWA поддержка (работа офлайн)
- [ ] WebSocket для real-time обновлений
- [ ] Многоязычность (EN/RU)
- [ ] Графики с историей за период
- [ ] Экспорт в PDF
- [ ] Уведомления браузера
- [ ] Голосовые команды

## Лицензия

MIT License - см. файл LICENSE в корне проекта
