#include <Arduino.h>
#include "SerialCompat.h"
#include "DisplayManager.h"
#include "config.h"

DisplayManager::DisplayManager(uint8_t w, uint8_t h, uint8_t addr)
    : width(w), height(h), currentMode(MODE_MEASUREMENT) {
    // Для дисплея 72x40 используем конструктор с явным указанием размера
    // -1 означает отсутствие reset пина (используется программный reset)
    display = new Adafruit_SSD1306(width, height, &Wire, -1);
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
    delay(100);
    
    // Для дисплея 72x40 используем offset
    int xOffset = OLED_OFFSET_X;
    int yOffset = OLED_OFFSET_Y;
    
    // Тестовая заливка для проверки работы дисплея (только физическая область 72x40)
    display->fillRect(xOffset, yOffset, OLED_PHYSICAL_WIDTH, OLED_PHYSICAL_HEIGHT, SSD1306_WHITE);
    display->display();
    delay(500);
    display->clearDisplay();
    display->display();
    delay(100);
    
    // Тест отображения текста в физической области дисплея
    display->setTextSize(1);
    // Максимум 12 символов на строку для 72px (72/6=12)
    display->setCursor(xOffset + 0, yOffset + 0);
    display->println("0123456789AB");  // 12 символов
    
    display->setCursor(xOffset + 0, yOffset + 10);
    display->println("Test 72x40");
    
    display->setCursor(xOffset + 0, yOffset + 20);
    display->print("W=72 H=40");
    
    display->display();
    delay(2000);
    display->clearDisplay();
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
    display->setCursor(xOffset + 0, yOffset + 0);
    display->println("Smart");
    display->setCursor(xOffset + 0, yOffset + 10);
    display->println("Areometr");

    display->setTextSize(1);
    display->setCursor(xOffset + 0, yOffset + 30);
    display->print("v");
    display->println(FIRMWARE_VERSION);

    display->display();
}

void DisplayManager::showMeasurement(float alcoholPercent, float temperature, bool isCompensated) {
    currentMode = MODE_MEASUREMENT;

    display->clearDisplay();
    
    // Для дисплея 72x40 используем offset
    int xOffset = OLED_OFFSET_X;
    int yOffset = OLED_OFFSET_Y;

    // Заголовок (компактный)
    display->setTextSize(1);
    display->setCursor(xOffset + 0, yOffset + 0);
    display->print("ALC:");
    display->drawLine(xOffset + 0, yOffset + 8, xOffset + OLED_PHYSICAL_WIDTH, yOffset + 8, SSD1306_WHITE);

    // Процент алкоголя
    display->setTextSize(1);
    display->setCursor(xOffset + 0, yOffset + 10);
    display->print(alcoholPercent, 1);
    display->println("%");

    // Температура
    display->setTextSize(1);
    display->setCursor(xOffset + 0, yOffset + 20);
    display->print("T:");
    display->print(temperature, 1);
    display->print((char)247);  // Символ градуса
    display->print("C");

    // Индикатор компенсации
    if (isCompensated) {
        display->setCursor(xOffset + 0, yOffset + 30);
        display->print("[TC]");
    }

    display->display();
}

void DisplayManager::showCalibration(uint8_t step, uint16_t value) {
    currentMode = MODE_CALIBRATION;

    display->clearDisplay();
    
    // Для дисплея 72x40 используем offset
    int xOffset = OLED_OFFSET_X;
    int yOffset = OLED_OFFSET_Y;

    display->setTextSize(1);
    display->setCursor(xOffset + 0, yOffset + 0);
    display->println("CALIBRATE");
    display->drawLine(xOffset + 0, yOffset + 8, xOffset + OLED_PHYSICAL_WIDTH, yOffset + 8, SSD1306_WHITE);

    display->setTextSize(1);
    display->setCursor(xOffset + 0, yOffset + 12);

    if (step == 0) {
        display->println("Step1:Water");
    } else if (step == 1) {
        display->println("Step2:Alco");
    }

    display->setCursor(xOffset + 0, yOffset + 24);
    display->print("Val:");
    display->println(value);

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
    
    // Первая строка: SSID (обрезаем до 12 символов)
    display->setCursor(xOffset + 0, yOffset + 0);
    String ssidShort = ssid;
    if (ssidShort.length() > 12) {
        ssidShort = ssidShort.substring(0, 12);
    }
    display->println(ssidShort);
    
    // Вторая строка: IP адрес (если длиннее 12 символов, переносим последние 3)
    display->setCursor(xOffset + 0, yOffset + 10);
    if (ip.length() > 12) {
        // Показываем первые символы (без последних 3)
        String ipFirst = ip.substring(0, ip.length() - 3);
        if (ipFirst.length() > 12) {
            ipFirst = ipFirst.substring(0, 12);
        }
        display->println(ipFirst);
        
        // Третья строка: последние 3 символа IP адреса
        display->setCursor(xOffset + 0, yOffset + 20);
        String ipLast = ip.substring(ip.length() - 3);
        display->println(ipLast);
        
        // Четвертая строка: Статус
        display->setCursor(xOffset + 0, yOffset + 30);
        String status = connected ? "Connected" : "AP Mode";
        if (status.length() > 12) {
            status = status.substring(0, 12);
        }
        display->println(status);
    } else {
        // IP адрес помещается на одну строку
        display->println(ip);
        
        // Третья строка: Статус
        display->setCursor(xOffset + 0, yOffset + 20);
        String status = connected ? "Connected" : "AP Mode";
        if (status.length() > 12) {
            status = status.substring(0, 12);
        }
        display->println(status);
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

    display->setTextSize(1);
    display->setCursor(xOffset + 0, yOffset + 0);
    display->println("ERROR!");
    display->drawLine(xOffset + 0, yOffset + 8, xOffset + OLED_PHYSICAL_WIDTH, yOffset + 8, SSD1306_WHITE);

    display->setTextSize(1);

    // Разбиваем длинное сообщение на строки для дисплея 72x40
    int lineHeight = 10;
    int y = yOffset + 12;
    int maxChars = 12;  // Максимум символов на строку для дисплея 72px (72/6=12)

    for (size_t i = 0; i < error.length(); i += maxChars) {
        String line = error.substring(i, min(i + maxChars, error.length()));
        display->setCursor(xOffset + 0, y);
        display->println(line);
        y += lineHeight;
        if (y > yOffset + OLED_PHYSICAL_HEIGHT - lineHeight) break;  // Не выходим за границы
    }

    display->display();
}

void DisplayManager::showMessage(const String &message, uint16_t duration) {
    display->clearDisplay();
    
    // Для дисплея 72x40 используем offset
    int xOffset = OLED_OFFSET_X;
    int yOffset = OLED_OFFSET_Y;

    display->setTextSize(1);
    
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
