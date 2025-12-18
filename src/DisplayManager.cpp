#include <Arduino.h>
#include "SerialCompat.h"
#include "DisplayManager.h"
#include "config.h"

DisplayManager::DisplayManager(uint8_t w, uint8_t h, uint8_t addr)
    : width(w), height(h), currentMode(MODE_MEASUREMENT), cycleIndex(0), lastCycleTime(0), lastDisplayUpdate(0), lastDisplayedCycleIndex(255) {
    // Для дисплея 72x40 используем конструктор с явным указанием размера
    // -1 означает отсутствие reset пина (используется программный reset)
    display = new Adafruit_SSD1306(width, height, &Wire, -1);
    savedAlcohol = 0.0f;
    savedTemperature = 20.0f;
    savedBatteryPercent = -1;
    lastDisplayedText = "";
}

DisplayManager::~DisplayManager() {
    delete display;
}

bool DisplayManager::begin() {
    // Инициализация I2C для ESP32-C3
    Serial.print("Initializing I2C on SDA=GPIO");
    Serial.print(OLED_SDA);
    Serial.print(", SCL=GPIO");
    Serial.println(OLED_SCL);
    
    // Инициализация I2C с частотой 100kHz (стандартная)
    Wire.begin(OLED_SDA, OLED_SCL);
    Wire.setClock(100000);  // 100kHz - стандартная частота I2C
    
    // Небольшая задержка для стабилизации I2C
    delay(100);
    
    // Проверка подключения дисплея (сканирование I2C)
    Serial.println("Scanning I2C bus...");
    byte error, address;
    int nDevices = 0;
    bool foundDisplay = false;
    
    for(address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        if (error == 0) {
            Serial.print("I2C device found at address 0x");
            if (address < 16) Serial.print("0");
            Serial.print(address, HEX);
            Serial.println(" !");
            nDevices++;
            if (address == OLED_ADDR || address == 0x3D) {
                foundDisplay = true;
            }
        }
    }
    
    if (nDevices == 0) {
        Serial.println("WARNING: No I2C devices found!");
        Serial.println("Check:");
        Serial.println("  1. I2C connections (SDA, SCL)");
        Serial.println("  2. Pull-up resistors (4.7kΩ on SDA and SCL to 3.3V)");
        Serial.println("  3. Power supply to display");
    } else if (!foundDisplay) {
        Serial.print("WARNING: Display not found at 0x");
        Serial.print(OLED_ADDR, HEX);
        Serial.println(" - trying alternative address 0x3D");
    }
    
    Serial.print("Initializing SSD1306 ");
    Serial.print(OLED_WIDTH);
    Serial.print("x");
    Serial.print(OLED_HEIGHT);
    Serial.print(" at address 0x");
    Serial.print(OLED_ADDR, HEX);
    Serial.println("...");

    // Инициализация дисплея - пробуем оба адреса и разные варианты
    bool initSuccess = false;
    
    // Вариант 1: Стандартная инициализация с SWITCHCAPVCC
    if (display->begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        initSuccess = true;
        Serial.print("Display initialized at 0x");
        Serial.println(OLED_ADDR, HEX);
        
        // Устанавливаем правильные команды для дисплея
        display->ssd1306_command(SSD1306_DISPLAYON);
        display->ssd1306_command(SSD1306_NORMALDISPLAY);
        // Устанавливаем смещение в 0 для начала с начала экрана
        display->ssd1306_command(SSD1306_SETDISPLAYOFFSET);
        display->ssd1306_command(0x00);  // Нет смещения
        // Устанавливаем правильную конфигурацию пинов
        if (width == 72 && height == 40) {
            display->ssd1306_command(SSD1306_SETCOMPINS);
            display->ssd1306_command(0x12);  // Для 72x40
        } else {
            display->ssd1306_command(SSD1306_SETCOMPINS);
            display->ssd1306_command(0x12);  // Для 128x64
        }
        
    } else {
        // Вариант 2: Альтернативный адрес 0x3D
        Serial.println("Trying alternative address 0x3D...");
        if (display->begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
            initSuccess = true;
            Serial.println("Display initialized at 0x3D");
            display->ssd1306_command(SSD1306_DISPLAYON);
            display->ssd1306_command(SSD1306_NORMALDISPLAY);
            display->ssd1306_command(SSD1306_SETDISPLAYOFFSET);
            display->ssd1306_command(0x00);
            display->ssd1306_command(SSD1306_SETCOMPINS);
            display->ssd1306_command(0x12);
        } else {
            // Вариант 3: Попробуем инициализацию без SWITCHCAPVCC
            Serial.println("Trying alternative initialization method...");
            if (display->begin(SSD1306_EXTERNALVCC, OLED_ADDR)) {
                initSuccess = true;
                Serial.println("Display initialized with EXTERNALVCC");
                display->ssd1306_command(SSD1306_DISPLAYON);
                display->ssd1306_command(SSD1306_NORMALDISPLAY);
                display->ssd1306_command(SSD1306_SETDISPLAYOFFSET);
                display->ssd1306_command(0x00);
                display->ssd1306_command(SSD1306_SETCOMPINS);
                display->ssd1306_command(0x12);
            }
        }
    }
    
    if (!initSuccess) {
        Serial.println("ERROR: SSD1306 initialization failed!");
        Serial.println("Troubleshooting:");
        Serial.print("  1. Check I2C connections (SDA=GPIO");
        Serial.print(OLED_SDA);
        Serial.print(", SCL=GPIO");
        Serial.println(OLED_SCL);
        Serial.println("  2. Check I2C address (try 0x3C or 0x3D)");
        Serial.println("  3. Check power supply (3.3V or 5V)");
        Serial.println("  4. Check pull-up resistors (4.7kΩ)");
        Serial.print("  5. Check display size (should be ");
        Serial.print(OLED_WIDTH);
        Serial.print("x");
        Serial.print(OLED_HEIGHT);
        Serial.println(")");
        return false;
    }

    Serial.println("Display initialized successfully");
    Serial.print("Display size: ");
    Serial.print(width);
    Serial.print("x");
    Serial.println(height);

    // Настройка дисплея
    display->clearDisplay();
    
    // Устанавливаем контраст для лучшей читаемости
    display->ssd1306_command(SSD1306_SETCONTRAST);
    display->ssd1306_command(255);  // Максимальный контраст
    
    // Нормальная ориентация (не инвертированная)
    display->ssd1306_command(SSD1306_NORMALDISPLAY);
    
    // ВАЖНО: Устанавливаем смещение в 0, чтобы текст начинался с начала экрана
    display->ssd1306_command(SSD1306_SETDISPLAYOFFSET);
    display->ssd1306_command(0x00);  // Нет смещения - начинаем с начала
    
    // Устанавливаем правильный диапазон колонок (0 до width-1)
    display->ssd1306_command(SSD1306_COLUMNADDR);
    display->ssd1306_command(0);   // Начальная колонка = 0
    display->ssd1306_command(width - 1);  // Конечная колонка
    
    // Устанавливаем правильный диапазон страниц (0 до height/8-1)
    display->ssd1306_command(SSD1306_PAGEADDR);
    display->ssd1306_command(0);   // Начальная страница = 0
    display->ssd1306_command((height / 8) - 1);  // Конечная страница
    
    display->setTextColor(SSD1306_WHITE);
    display->setTextSize(1);
    display->display();

    return true;
}

void DisplayManager::clear() {
    display->clearDisplay();
    display->display();
}

void DisplayManager::showBootScreen() {
    display->clearDisplay();
    
    // Для дисплея 72x40 используем offset согласно https://github.com/peff74/ESP32-C3_OLED
    int xOffset = OLED_OFFSET_X;
    int yOffset = OLED_OFFSET_Y;
    
    display->setTextSize(1);
    
    // Центрируем название проекта - "Smart" на первой строке
    String line1 = "Smart";
    int line1X = xOffset + (OLED_PHYSICAL_WIDTH - line1.length() * 6) / 2;
    if (line1X < xOffset) line1X = xOffset;
    display->setCursor(line1X, yOffset + 8);
    display->println(line1);

    // "Areometr" на второй строке
    String line2 = "Areometr";
    int line2X = xOffset + (OLED_PHYSICAL_WIDTH - line2.length() * 6) / 2;
    if (line2X < xOffset) line2X = xOffset;
    display->setCursor(line2X, yOffset + 18);
    display->println(line2);

    // Центрируем версию
    String version = "v" + String(FIRMWARE_VERSION);
    int versionX = xOffset + (OLED_PHYSICAL_WIDTH - version.length() * 6) / 2;
    if (versionX < xOffset) versionX = xOffset;
    display->setCursor(versionX, yOffset + 32);
    display->println(version);

    display->display();
}

void DisplayManager::showMeasurement(float alcoholPercent, float temperature, bool isCompensated, int8_t batteryPercent, float batteryVoltage) {
    // Не перезаписываем экран, если показывается ошибка
    if (currentMode == MODE_ERROR) {
        return;
    }
    
    currentMode = MODE_MEASUREMENT;

    // Сохраняем значения
    savedAlcohol = alcoholPercent;
    savedTemperature = temperature;
    savedBatteryPercent = batteryPercent;

    // Проверяем, нужно ли переключить экран
    unsigned long currentTime = millis();
    bool cycleChanged = false;
    if (currentTime - lastCycleTime >= CYCLE_INTERVAL) {
        cycleIndex = (cycleIndex + 1) % 3;  // Цикл: 0->1->2->0
        lastCycleTime = currentTime;
        cycleChanged = true;
    }

    // Определяем текст для отображения без перерисовки
    String displayText = "";
    String labelText = "";

    // Определяем, что показывать в зависимости от cycleIndex
    switch (cycleIndex) {
        case 0:  // Крепость (алкоголь)
            labelText = "ALC";
            // Форматируем: если >= 100, показываем "100%", иначе с 1 знаком после запятой
            if (alcoholPercent >= 100.0f) {
                displayText = "100%";
            } else if (alcoholPercent >= 10.0f) {
                // Для значений >= 10 показываем целое число, чтобы поместилось в 4 символа
                displayText = String((int)alcoholPercent) + "%";
            } else {
                // Для значений < 10 показываем с 1 знаком после запятой (например "9.5%")
                displayText = String(alcoholPercent, 1) + "%";
            }
            break;

        case 1:  // Температура
            labelText = "TEMP";
            // Показываем температуру с 1 знаком после запятой и символом градуса
            if (temperature >= 100.0f || temperature < 0.0f) {
                // Если >= 100 или < 0, показываем целое число
                displayText = String((int)temperature) + String((char)247) + "C";  // 247 = символ градуса
            } else {
                // Для нормальных значений показываем с 1 знаком
                displayText = String(temperature, 1) + String((char)247) + "C";
            }
            break;

        case 2:  // Заряд батареи
            labelText = "BATT";
            if (batteryPercent >= 0) {
                displayText = String(batteryPercent) + "%";
            } else {
                displayText = "N/A";
            }
            break;
    }

    // Проверяем, нужно ли обновлять дисплей
    // Обновляем, если: изменился цикл, изменился текст, или прошло достаточно времени с последнего обновления
    bool needsUpdate = cycleChanged || 
                      (lastDisplayedCycleIndex != cycleIndex) || 
                      (lastDisplayedText != displayText) ||
                      (currentTime - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL);

    if (!needsUpdate) {
        return;  // Не перерисовываем, если ничего не изменилось
    }

    // Обновляем время последнего обновления
    lastDisplayUpdate = currentTime;
    lastDisplayedCycleIndex = cycleIndex;
    lastDisplayedText = displayText;

    display->clearDisplay();

    // Для дисплея 72x40 используем offset
    int xOffset = OLED_OFFSET_X;
    int yOffset = OLED_OFFSET_Y;

    // Используем максимально крупный шрифт (textSize 3 = 18x24 пикселей на символ)
    // Для дисплея 72x40: максимально 4 символа по ширине, 1.5 строки по высоте
    display->setTextSize(3);
    display->setTextColor(SSD1306_WHITE);

    // Если текст слишком длинный для textSize 3 (более 4 символов), используем textSize 2
    uint8_t textSize = 3;
    if (displayText.length() > 4) {
        textSize = 2;  // textSize 2 = 12x16 пикселей, до 6 символов по ширине
    }
    
    display->setTextSize(textSize);
    
    // Вычисляем позицию для центрирования текста
    int charWidth = (textSize == 3) ? 18 : 12;
    int charHeight = (textSize == 3) ? 24 : 16;
    int textWidth = displayText.length() * charWidth;
    int textX = xOffset + (OLED_PHYSICAL_WIDTH - textWidth) / 2;
    if (textX < xOffset) textX = xOffset;  // Не выходим за левую границу
    
    // Центрируем по вертикали (высота экрана 40px) и опускаем на 6 пикселей ниже
    int textY = yOffset + (OLED_PHYSICAL_HEIGHT - charHeight) / 2 + 6;
    
    // Если есть метка, показываем её мелким шрифтом сверху
    if (!labelText.isEmpty()) {
        display->setTextSize(1);
        int labelX = xOffset + (OLED_PHYSICAL_WIDTH - labelText.length() * 6) / 2;
        if (labelX < xOffset) labelX = xOffset;
        display->setCursor(labelX, yOffset + 2);
        display->print(labelText);
        
        // Возвращаем крупный шрифт для основного текста
        display->setTextSize(textSize);
    }
    
    // Показываем основное значение (цифры)
    display->setCursor(textX, textY);
    display->print(displayText);
    
    display->display();
}

void DisplayManager::showCalibration(uint8_t step, uint16_t value) {
    currentMode = MODE_CALIBRATION;

    display->clearDisplay();
    
    // Для дисплея 72x40 используем offset
    int xOffset = OLED_OFFSET_X;
    int yOffset = OLED_OFFSET_Y;

    display->setTextSize(1);
    
    // "CALIBRATE" по центру
    String calibrateLabel = "CALIBRATE";
    int calibrateX = xOffset + (OLED_PHYSICAL_WIDTH - calibrateLabel.length() * 6) / 2;
    if (calibrateX < xOffset) calibrateX = xOffset;
    display->setCursor(calibrateX, yOffset + 0);
    display->println(calibrateLabel);

    // Шаг калибровки по центру, опущен на строку ниже
    String stepText = "";
    if (step == 0) {
        stepText = "Step1:Water";
    } else if (step == 1) {
        stepText = "Step2:Alco";
    }
    if (!stepText.isEmpty()) {
        int stepX = xOffset + (OLED_PHYSICAL_WIDTH - stepText.length() * 6) / 2;
        if (stepX < xOffset) stepX = xOffset;
        display->setCursor(stepX, yOffset + 18);
        display->println(stepText);
    }

    // Значение по центру
    String valueText = "Val:" + String(value);
    int valueX = xOffset + (OLED_PHYSICAL_WIDTH - valueText.length() * 6) / 2;
    if (valueX < xOffset) valueX = xOffset;
    display->setCursor(valueX, yOffset + 30);
    display->println(valueText);

    display->display();
}

void DisplayManager::showNetworkInfo(const String &ssid, const String &ip, bool connected) {
    currentMode = MODE_SETTINGS;

    display->clearDisplay();
    display->setTextSize(1);
    
    // ВАЖНО: Для дисплея 72x40 нужно использовать offset
    // Контроллер SSD1306 работает как 128x64, но физический дисплей 72x40
    // Используем смещение для правильного позиционирования
    int xOffset = OLED_OFFSET_X;
    int yOffset = OLED_OFFSET_Y;
    
    // Максимум 12 символов на строку для физической ширины 72px (72px / 6px на символ)
    
    // Первая строка: "WiFi:" по центру
    String wifiLabel = "WiFi:";
    int wifiLabelX = xOffset + (OLED_PHYSICAL_WIDTH - wifiLabel.length() * 6) / 2;
    if (wifiLabelX < xOffset) wifiLabelX = xOffset;
    display->setCursor(wifiLabelX, yOffset + 0);
    display->println(wifiLabel);
    
    // Вторая строка: SSID (обрезаем до 12 символов) по центру
    String ssidShort = ssid;
    if (ssidShort.length() > 12) {
        ssidShort = ssidShort.substring(0, 12);
    }
    int ssidX = xOffset + (OLED_PHYSICAL_WIDTH - ssidShort.length() * 6) / 2;
    if (ssidX < xOffset) ssidX = xOffset;
    display->setCursor(ssidX, yOffset + 10);
    display->println(ssidShort);
    
    // Третья строка: IP адрес (если длиннее 12 символов, переносим последние 3) по центру
    display->setCursor(xOffset + 0, yOffset + 20);
    if (ip.length() > 12) {
        // Показываем первые символы (без последних 3)
        String ipFirst = ip.substring(0, ip.length() - 3);
        if (ipFirst.length() > 12) {
            ipFirst = ipFirst.substring(0, 12);
        }
        int ipFirstX = xOffset + (OLED_PHYSICAL_WIDTH - ipFirst.length() * 6) / 2;
        if (ipFirstX < xOffset) ipFirstX = xOffset;
        display->setCursor(ipFirstX, yOffset + 20);
        display->println(ipFirst);
        
        // Четвертая строка: последние 3 символа IP адреса по центру
        String ipLast = ip.substring(ip.length() - 3);
        int ipLastX = xOffset + (OLED_PHYSICAL_WIDTH - ipLast.length() * 6) / 2;
        if (ipLastX < xOffset) ipLastX = xOffset;
        display->setCursor(ipLastX, yOffset + 30);
        display->println(ipLast);
    } else {
        // IP адрес помещается на одну строку - центрируем
        int ipX = xOffset + (OLED_PHYSICAL_WIDTH - ip.length() * 6) / 2;
        if (ipX < xOffset) ipX = xOffset;
        display->setCursor(ipX, yOffset + 20);
        display->println(ip);
    }

    display->display();
}

void DisplayManager::showError(const String &error) {
    currentMode = MODE_ERROR;
    lastError = error;

    display->clearDisplay();
    
    // Для дисплея 72x40 используем offset
    int xOffset = OLED_OFFSET_X;
    int yOffset = OLED_OFFSET_Y;

    // "ERROR!" крупным шрифтом и по центру
    display->setTextSize(2);
    String errorLabel = "ERROR!";
    int errorLabelWidth = errorLabel.length() * 12;  // textSize 2 = 12px на символ
    int errorLabelX = xOffset + (OLED_PHYSICAL_WIDTH - errorLabelWidth) / 2;
    if (errorLabelX < xOffset) errorLabelX = xOffset;
    display->setCursor(errorLabelX, yOffset + 0);
    display->println(errorLabel);

    // Текст ошибки по центру
    display->setTextSize(1);
    
    // Специальная обработка для "Not calibrated!"
    if (error.indexOf("Not") >= 0 && error.indexOf("calibrated") >= 0) {
        String line1 = "Not";
        String line2 = "calibrated";
        
        int line1X = xOffset + (OLED_PHYSICAL_WIDTH - line1.length() * 6) / 2;
        if (line1X < xOffset) line1X = xOffset;
        display->setCursor(line1X, yOffset + 18);
        display->println(line1);
        
        int line2X = xOffset + (OLED_PHYSICAL_WIDTH - line2.length() * 6) / 2;
        if (line2X < xOffset) line2X = xOffset;
        display->setCursor(line2X, yOffset + 28);
        display->println(line2);
    } else {
        // Обычная обработка - разбиваем на строки
        int lineHeight = 10;
        int y = yOffset + 18;
        int maxChars = 12;  // Максимум символов на строку для дисплея 72px (72/6=12)

        for (size_t i = 0; i < error.length(); i += maxChars) {
            String line = error.substring(i, min(i + maxChars, error.length()));
            int lineX = xOffset + (OLED_PHYSICAL_WIDTH - line.length() * 6) / 2;
            if (lineX < xOffset) lineX = xOffset;
            display->setCursor(lineX, y);
            display->println(line);
            y += lineHeight;
            if (y > yOffset + OLED_PHYSICAL_HEIGHT - lineHeight) break;  // Не выходим за границы
        }
    }

    display->display();
}

void DisplayManager::showMessage(const String &message, uint16_t duration) {
    display->clearDisplay();
    
    // Для дисплея 72x40 используем offset
    int xOffset = OLED_OFFSET_X;
    int yOffset = OLED_OFFSET_Y;

    display->setTextSize(1);
    
    // Специальная обработка для "Starting Wi-Fi..."
    if (message.indexOf("Starting") >= 0 && message.indexOf("Wi-Fi") >= 0) {
        String line1 = "Starting";
        String line2 = "wi-fi";
        
        int line1X = xOffset + (OLED_PHYSICAL_WIDTH - line1.length() * 6) / 2;
        if (line1X < xOffset) line1X = xOffset;
        display->setCursor(line1X, yOffset + 12);
        display->println(line1);
        
        int line2X = xOffset + (OLED_PHYSICAL_WIDTH - line2.length() * 6) / 2;
        if (line2X < xOffset) line2X = xOffset;
        display->setCursor(line2X, yOffset + 22);
        display->println(line2);
    } else {
        // Обрезаем длинные сообщения для дисплея 72px (max 12 символов)
        String msg = message;
        if (msg.length() > 12) {
            msg = msg.substring(0, 12);
        }
        
        // Центрируем текст в физической области 72x40
        int x = xOffset + (OLED_PHYSICAL_WIDTH - msg.length() * 6) / 2;
        int y = yOffset + (OLED_PHYSICAL_HEIGHT / 2) - 4;
        if (x < xOffset) x = xOffset;
        
        display->setCursor(x, y);
        display->println(msg);
    }
    
    display->display();

    if (duration > 0) {
        delay(duration);
    }
}

void DisplayManager::update() {
    // Здесь можно добавить анимации или обновление динамического контента
}

void DisplayManager::setBrightness(uint8_t brightness) {
    // SSD1306 поддерживает команду контрастности
    // 0 = минимум, 255 = максимум
    display->ssd1306_command(SSD1306_SETCONTRAST);
    display->ssd1306_command(brightness);
}

bool DisplayManager::isShowingError() const {
    return currentMode == MODE_ERROR;
}
