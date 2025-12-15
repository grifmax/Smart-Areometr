# Примеры использования

## Базовые сценарии

### 1. Измерение крепости самогона

**Задача:** Определить крепость самогона после второй перегонки

**Процедура:**
1. Убедитесь, что устройство откалибровано
2. Налейте самогон в чистый стакан (минимум 50мл)
3. Погрузите датчик в жидкость (глубина ~3см)
4. Подождите 5-10 секунд для стабилизации температуры
5. Считайте показания с дисплея или веб-интерфейса
6. Запишите результат

**Пример результата:**
```
Alcohol: 68.5%
Temperature: 21.2°C
[TC] - температурная компенсация применена
```

---

### 2. Контроль разбавления спирта

**Задача:** Разбавить 96% спирт до 40%

**Процедура:**
1. Измерьте исходную крепость спирта
2. Добавляйте воду небольшими порциями
3. Перемешивайте после каждого добавления
4. Измеряйте крепость
5. Продолжайте до достижения целевого значения

**Калькулятор разбавления:**
```
Формула: V_воды = V_спирта × (C_исх - C_цел) / C_цел

Где:
- V_воды - объем воды для добавления
- V_спирта - объем исходного спирта
- C_исх - исходная крепость (%)
- C_цел - целевая крепость (%)

Пример:
Есть 1л 96% спирта, нужно получить 40%
V_воды = 1000 × (96 - 40) / 40 = 1400 мл
```

---

### 3. Мониторинг брожения

**Задача:** Отслеживать процесс брожения браги

**Процедура:**
1. Измеряйте крепость браги каждые 6-12 часов
2. Записывайте данные в лог
3. Стройте график изменения крепости
4. Брожение завершено когда крепость стабилизируется

**Анализ логов:**
```python
import json
import matplotlib.pyplot as plt
from datetime import datetime

# Загрузить данные с устройства
import requests
response = requests.get('http://192.168.4.1/api/logs')
data = response.json()

# Построить график
timestamps = [m['timestamp']/1000/3600 for m in data['measurements']]  # в часах
alcohol = [m['alcohol'] for m in data['measurements']]

plt.plot(timestamps, alcohol)
plt.xlabel('Время (часы)')
plt.ylabel('Крепость (%)')
plt.title('Процесс брожения')
plt.grid(True)
plt.show()
```

---

## Продвинутые сценарии

### 4. Создание калибровочной кривой

Для улучшения точности можно создать многоточечную калибровку.

**Процедура:**
1. Подготовьте эталонные растворы известной крепости:
   - 0% (вода)
   - 20%, 40%, 60%, 80%, 100% (спирт + вода)

2. Измерьте сырые значения датчика для каждого раствора
3. Постройте калибровочную кривую
4. Используйте интерполяцию для точных измерений

**Пример кода:**
```python
import numpy as np
from scipy.interpolate import interp1d

# Данные калибровки
alcohol_percent = [0, 20, 40, 60, 80, 100]
raw_values = [150, 135, 115, 95, 85, 75]  # Примерные значения

# Создать интерполяционную функцию
calibration_curve = interp1d(raw_values, alcohol_percent, kind='cubic')

# Использовать для измерения
measured_raw = 105
actual_percent = calibration_curve(measured_raw)
print(f"Alcohol: {actual_percent:.1f}%")
```

**Сохранение в устройство:**
Модифицируйте `CapacitiveSensor.cpp` для использования массива калибровочных точек.

---

### 5. Температурная компенсация

**Влияние температуры на измерения:**
- При повышении температуры показания занижаются
- При понижении - завышаются
- Коэффициент: ~0.4% на градус Цельсия

**Эксперимент:**
1. Измерьте раствор при 15°C, 20°C, 25°C
2. Запишите показания
3. Рассчитайте коэффициент компенсации
4. Обновите `TEMP_COEFFICIENT` в config.h

**Формула компенсации:**
```
Alcohol_compensated = Alcohol_raw - (T_actual - T_reference) × K

Где:
- Alcohol_raw - измеренное значение
- T_actual - текущая температура
- T_reference - эталонная температура (20°C)
- K - коэффициент компенсации (0.4%/°C)
```

---

### 6. Интеграция с умным домом (Home Assistant)

**Конфигурация sensors:**
```yaml
# configuration.yaml

sensor:
  # Процент алкоголя
  - platform: rest
    name: "Areometr Alcohol"
    resource: http://192.168.1.100/api/measurement
    value_template: '{{ value_json.alcohol | round(1) }}'
    unit_of_measurement: '%'
    scan_interval: 30
    device_class: none

  # Температура
  - platform: rest
    name: "Areometr Temperature"
    resource: http://192.168.1.100/api/measurement
    value_template: '{{ value_json.temperature | round(1) }}'
    unit_of_measurement: '°C'
    device_class: temperature
    scan_interval: 30

  # Статус калибровки
  - platform: rest
    name: "Areometr Calibrated"
    resource: http://192.168.1.100/api/status
    value_template: '{{ value_json.calibrated }}'
    scan_interval: 300
```

**Автоматизация - уведомление о завершении брожения:**
```yaml
automation:
  - alias: "Areometr - Fermentation Complete"
    trigger:
      platform: numeric_state
      entity_id: sensor.areometr_alcohol
      above: 12
      for:
        hours: 6
    action:
      - service: notify.mobile_app
        data:
          title: "Брожение завершено!"
          message: "Крепость: {{ states('sensor.areometr_alcohol') }}%"
```

---

### 7. Telegram бот для мониторинга

**Python скрипт:**
```python
import requests
import telebot
from time import sleep

BOT_TOKEN = 'your_bot_token'
CHAT_ID = 'your_chat_id'
DEVICE_IP = '192.168.1.100'

bot = telebot.TeleBot(BOT_TOKEN)

@bot.message_handler(commands=['measure'])
def send_measurement(message):
    try:
        response = requests.get(f'http://{DEVICE_IP}/api/measurement')
        data = response.json()

        text = f"🍺 Измерение:\n"
        text += f"Крепость: {data['alcohol']:.1f}%\n"
        text += f"Температура: {data['temperature']:.1f}°C\n"
        text += f"Калибровка: {'✅' if data['calibrated'] else '❌'}"

        bot.send_message(message.chat.id, text)
    except Exception as e:
        bot.send_message(message.chat.id, f"Ошибка: {e}")

@bot.message_handler(commands=['calibrate'])
def calibrate(message):
    bot.send_message(message.chat.id, "Начинаю калибровку...")
    # Отправить команды калибровки
    requests.post(f'http://{DEVICE_IP}/api/calibrate/water')
    sleep(5)
    requests.post(f'http://{DEVICE_IP}/api/calibrate/alcohol')
    bot.send_message(message.chat.id, "Калибровка завершена!")

print("Bot started...")
bot.polling(none_stop=True)
```

---

### 8. Экспорт данных в Excel

**Python скрипт для экспорта логов:**
```python
import requests
import pandas as pd
from datetime import datetime

DEVICE_IP = '192.168.1.100'

# Получить данные
response = requests.get(f'http://{DEVICE_IP}/api/logs')
data = response.json()

# Преобразовать в DataFrame
df = pd.DataFrame(data['measurements'])
df['datetime'] = pd.to_datetime(df['timestamp'], unit='ms')
df = df[['datetime', 'alcohol', 'temperature', 'compensated']]

# Сохранить в Excel
filename = f"areometr_log_{datetime.now().strftime('%Y%m%d_%H%M%S')}.xlsx"
df.to_excel(filename, index=False)

print(f"Данные экспортированы в {filename}")
```

---

### 9. Графический интерфейс на Raspberry Pi

**Python + Tkinter:**
```python
import tkinter as tk
import requests
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg

DEVICE_IP = '192.168.1.100'

class AreometrGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Smart Areometr Monitor")

        # Labels
        self.alcohol_label = tk.Label(root, text="Alcohol: --", font=("Arial", 24))
        self.alcohol_label.pack(pady=10)

        self.temp_label = tk.Label(root, text="Temperature: --", font=("Arial", 18))
        self.temp_label.pack(pady=10)

        # Buttons
        tk.Button(root, text="Measure", command=self.measure).pack(pady=5)
        tk.Button(root, text="Calibrate", command=self.calibrate).pack(pady=5)

        # Graph
        self.fig = Figure(figsize=(6, 4))
        self.ax = self.fig.add_subplot(111)
        self.canvas = FigureCanvasTkAgg(self.fig, master=root)
        self.canvas.get_tk_widget().pack()

        # Auto-update
        self.update_data()

    def measure(self):
        try:
            response = requests.get(f'http://{DEVICE_IP}/api/measurement')
            data = response.json()

            self.alcohol_label.config(text=f"Alcohol: {data['alcohol']:.1f}%")
            self.temp_label.config(text=f"Temperature: {data['temperature']:.1f}°C")
        except Exception as e:
            print(f"Error: {e}")

    def calibrate(self):
        # Implement calibration
        pass

    def update_data(self):
        self.measure()
        self.root.after(5000, self.update_data)  # Update every 5 seconds

root = tk.Tk()
app = AreometrGUI(root)
root.mainloop()
```

---

## Советы и рекомендации

### Точность измерений

1. **Перед измерением:**
   - Убедитесь, что датчик чистый
   - Проверьте температуру раствора (оптимально 15-25°C)
   - Дайте раствору отстояться (без пузырьков)

2. **Во время измерения:**
   - Не перемешивайте раствор
   - Держите датчик неподвижно
   - Подождите стабилизации показаний (5-10 сек)

3. **После измерения:**
   - Промойте датчик водой
   - Высушите перед хранением

### Частые ошибки

**Показания скачут:**
- Причина: пузырьки воздуха на электродах
- Решение: слегка постучите по датчику

**Всегда показывает 0% или 100%:**
- Причина: нарушена калибровка
- Решение: выполните повторную калибровку

**Большая погрешность:**
- Причина: температура сильно отличается от эталонной
- Решение: дождитесь стабилизации температуры

### Обслуживание

**Еженедельно:**
- Очистка датчиков спиртом
- Проверка калибровки на воде

**Ежемесячно:**
- Полная повторная калибровка
- Проверка точности на известных растворах

**При необходимости:**
- Обновление прошивки
- Замена изношенных электродов
