#include <Arduino.h>
#include "SerialCompat.h"
#include "config.h"
#include "ADS1115Driver.h"

ADS1115Driver::ADS1115Driver(uint8_t address)
    : i2cAddress(address), currentGain(GAIN_FOUR), dataRate(ADS1115_DATA_RATE), initialized(false) {
    ads = new Adafruit_ADS1115();
}

ADS1115Driver::~ADS1115Driver() {
    if (ads) {
        delete ads;
        ads = nullptr;
    }
}

bool ADS1115Driver::begin() {
    Serial.print("Initializing ADS1115 at address 0x");
    Serial.print(i2cAddress, HEX);
    Serial.println("...");
    
    // Инициализация устройства
    if (!ads->begin(i2cAddress)) {
        Serial.println("ERROR: ADS1115 not found! Check wiring and I2C address.");
        initialized = false;
        return false;
    }
    
    // Установка параметров
    ads->setGain(currentGain);
    
    // Проверка подключения
    if (!checkConnection()) {
        Serial.println("ERROR: ADS1115 connection test failed!");
        initialized = false;
        return false;
    }
    
    initialized = true;
    
    Serial.print("ADS1115 initialized successfully!");
    Serial.print(" Gain: ");
    Serial.print(getMaxVoltage(), 3);
    Serial.print("V, Data rate: ");
    Serial.print(dataRate);
    Serial.println(" SPS");
    
    return true;
}

bool ADS1115Driver::checkConnection() {
    // Попытка прочитать регистр конфигурации
    Wire.beginTransmission(i2cAddress);
    uint8_t error = Wire.endTransmission();
    return (error == 0);
}

bool ADS1115Driver::isConnected() {
    if (!initialized) {
        return false;
    }
    return checkConnection();
}

int16_t ADS1115Driver::readDifferential(uint8_t channel1, uint8_t channel2) {
    if (!initialized || !ads) {
        Serial.println("ERROR: ADS1115 not initialized!");
        return 0;
    }
    
    // Валидация каналов
    if (channel1 > 3 || channel2 > 3) {
        Serial.println("ERROR: Invalid channel number (must be 0-3)!");
        return 0;
    }
    
    // Чтение дифференциального значения
    // Библиотека Adafruit использует специальные константы для дифференциальных каналов
    int16_t value = 0;
    
    // Маппинг каналов на дифференциальные пары библиотеки Adafruit
    // ADS1X15_DIFF_P0_N1 = канал 0 и 1
    // ADS1X15_DIFF_P0_N3 = канал 0 и 3
    // ADS1X15_DIFF_P1_N3 = канал 1 и 3
    // ADS1X15_DIFF_P2_N3 = канал 2 и 3
    
    if (channel1 == 0 && channel2 == 1) {
        value = ads->readADC_Differential_0_1();
    } else if (channel1 == 0 && channel2 == 3) {
        value = ads->readADC_Differential_0_3();
    } else if (channel1 == 1 && channel2 == 3) {
        value = ads->readADC_Differential_1_3();
    } else if (channel1 == 2 && channel2 == 3) {
        value = ads->readADC_Differential_2_3();
    } else {
        // Для других комбинаций используем общий метод
        // Библиотека Adafruit не поддерживает все комбинации напрямую
        // Используем одиночные каналы и вычисляем разность программно
        int16_t val1 = ads->readADC_SingleEnded(channel1);
        int16_t val2 = ads->readADC_SingleEnded(channel2);
        value = val1 - val2;
    }
    
    return value;
}

int16_t ADS1115Driver::readSingleEnded(uint8_t channel) {
    if (!initialized || !ads) {
        Serial.println("ERROR: ADS1115 not initialized!");
        return 0;
    }
    
    if (channel > 3) {
        Serial.println("ERROR: Invalid channel number (must be 0-3)!");
        return 0;
    }
    
    return ads->readADC_SingleEnded(channel);
}

void ADS1115Driver::setGain(adsGain_t gain) {
    currentGain = gain;
    if (ads && initialized) {
        ads->setGain(gain);
        Serial.print("ADS1115 gain set to: ");
        Serial.print(getMaxVoltage(), 3);
        Serial.println("V");
    }
}

void ADS1115Driver::setDataRate(uint16_t rate) {
    dataRate = rate;
    // Примечание: Библиотека Adafruit не предоставляет прямой метод установки скорости
    // Это можно сделать через прямое обращение к регистрам, но для начала используем значения по умолчанию
    Serial.print("ADS1115 data rate set to: ");
    Serial.print(rate);
    Serial.println(" SPS (note: may require direct register access)");
}

float ADS1115Driver::getVoltage(int16_t rawValue) {
    if (!ads || !initialized) {
        return 0.0f;
    }
    return ads->computeVolts(rawValue);
}

float ADS1115Driver::getMaxVoltage() const {
    // Максимальное напряжение зависит от усиления
    // Для ADS1115 доступны: GAIN_TWOTHIRDS, GAIN_ONE, GAIN_TWO, GAIN_FOUR, GAIN_EIGHT
    switch (currentGain) {
        case GAIN_TWOTHIRDS:  // ±6.144V (для ADS1015)
            return 6.144f;
        case GAIN_ONE:        // ±4.096V
            return 4.096f;
        case GAIN_TWO:        // ±2.048V
            return 2.048f;
        case GAIN_FOUR:       // ±1.024V (рекомендуется для малых сигналов)
            return 1.024f;
        case GAIN_EIGHT:      // ±0.512V
            return 0.512f;
        default:
            return 1.024f;  // По умолчанию GAIN_FOUR
    }
}

bool ADS1115Driver::testConnection() {
    if (!initialized) {
        Serial.println("ADS1115 test: Not initialized");
        return false;
    }
    
    Serial.println("Testing ADS1115 connection...");
    
    // Проверка подключения
    if (!checkConnection()) {
        Serial.println("ADS1115 test: I2C connection failed");
        return false;
    }
    
    // Попытка прочитать значение с канала 0
    int16_t testValue = readSingleEnded(0);
    float voltage = getVoltage(testValue);
    
    Serial.print("ADS1115 test: Channel 0 reading = ");
    Serial.print(testValue);
    Serial.print(" (");
    Serial.print(voltage, 4);
    Serial.println("V)");
    
    // Проверка, что значение в разумных пределах
    if (abs(testValue) > 32767) {
        Serial.println("ADS1115 test: Reading out of range");
        return false;
    }
    
    Serial.println("ADS1115 test: PASSED");
    return true;
}
