#include <Arduino.h>
#include "DisplayManager.h"
#include "config.h"

DisplayManager::DisplayManager(uint8_t w, uint8_t h, uint8_t addr)
    : width(w), height(h), currentMode(MODE_MEASUREMENT) {
    display = new Adafruit_SSD1306(width, height, &Wire, -1);
}

DisplayManager::~DisplayManager() {
    delete display;
}

bool DisplayManager::begin() {
    // Инициализация I2C
    Wire.begin(OLED_SDA, OLED_SCL);

    // Инициализация дисплея
    if (!display->begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("ERROR: SSD1306 allocation failed");
        return false;
    }

    Serial.println("Display initialized");

    // Настройка дисплея
    display->clearDisplay();
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
    display->setTextSize(2);
    display->setCursor(0, 10);
    display->println("Smart");
    display->println("Areometr");

    display->setTextSize(1);
    display->setCursor(0, 50);
    display->print("v");
    display->println(FIRMWARE_VERSION);

    display->display();
}

void DisplayManager::showMeasurement(float alcoholPercent, float temperature, bool isCompensated) {
    currentMode = MODE_MEASUREMENT;

    display->clearDisplay();

    // Заголовок
    display->setTextSize(1);
    display->setCursor(0, 0);
    display->println("ALCOHOL METER");
    display->drawLine(0, 10, width, 10, SSD1306_WHITE);

    // Процент алкоголя (большими буквами)
    display->setTextSize(2);
    display->setCursor(0, 20);
    display->print(alcoholPercent, 1);
    display->println("%");

    // Температура
    display->setTextSize(1);
    display->setCursor(0, 45);
    display->print("Temp: ");
    display->print(temperature, 1);
    display->print((char)247);  // Символ градуса
    display->println("C");

    // Индикатор компенсации
    if (isCompensated) {
        display->setCursor(width - 24, 45);
        display->println("[TC]");
    }

    display->display();
}

void DisplayManager::showCalibration(uint8_t step, uint16_t value) {
    currentMode = MODE_CALIBRATION;

    display->clearDisplay();

    display->setTextSize(1);
    display->setCursor(0, 0);
    display->println("CALIBRATION");
    display->drawLine(0, 10, width, 10, SSD1306_WHITE);

    display->setTextSize(1);
    display->setCursor(0, 20);

    if (step == 0) {
        display->println("Step 1: Water");
        display->println("Place sensor in");
        display->println("pure water");
    } else if (step == 1) {
        display->println("Step 2: Alcohol");
        display->println("Place sensor in");
        display->println("pure alcohol");
    }

    display->setCursor(0, 55);
    display->print("Value: ");
    display->println(value);

    display->display();
}

void DisplayManager::showNetworkInfo(const String &ssid, const String &ip, bool connected) {
    currentMode = MODE_SETTINGS;

    display->clearDisplay();

    display->setTextSize(1);
    display->setCursor(0, 0);
    display->println("NETWORK INFO");
    display->drawLine(0, 10, width, 10, SSD1306_WHITE);

    display->setCursor(0, 15);
    display->print("Status: ");
    display->println(connected ? "Connected" : "Disconnected");

    display->setCursor(0, 30);
    display->print("SSID: ");
    display->println(ssid);

    display->setCursor(0, 45);
    display->print("IP: ");
    display->println(ip);

    display->display();
}

void DisplayManager::showError(const String &error) {
    currentMode = MODE_ERROR;
    lastError = error;

    display->clearDisplay();

    display->setTextSize(1);
    display->setCursor(0, 0);
    display->println("ERROR!");
    display->drawLine(0, 10, width, 10, SSD1306_WHITE);

    display->setCursor(0, 20);
    display->setTextSize(1);

    // Разбиваем длинное сообщение на строки
    int lineHeight = 10;
    int y = 20;
    int maxChars = 21;  // Максимум символов на строку для размера 1

    for (size_t i = 0; i < error.length(); i += maxChars) {
        String line = error.substring(i, min(i + maxChars, error.length()));
        display->setCursor(0, y);
        display->println(line);
        y += lineHeight;
        if (y > height - lineHeight) break;  // Не выходим за границы
    }

    display->display();
}

void DisplayManager::showMessage(const String &message, uint16_t duration) {
    display->clearDisplay();

    display->setTextSize(1);
    display->setCursor(0, height / 2 - 8);

    // Центрируем текст если он короткий
    if (message.length() <= 21) {
        int x = (width - message.length() * 6) / 2;
        display->setCursor(x, height / 2 - 4);
    }

    display->println(message);
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
