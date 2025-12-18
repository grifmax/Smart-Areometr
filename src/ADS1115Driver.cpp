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
    
    // Инициализация I2C если еще не инициализирован
    if (!Wire.begin()) {
        Serial.println("ERROR: I2C initialization failed!");
        initialized = false;
        return false;
    }
    
    // Проверка подключения перед инициализацией библиотеки
    if (!checkConnection()) {
        Serial.println("ERROR: ADS1115 not found on I2C bus! Check wiring and I2C address.");
        initialized = false;
        return false;
    }
    
    // Инициализация устройства
    if (!ads->begin(i2cAddress)) {
        Serial.println("ERROR: ADS1115 initialization failed!");
        initialized = false;
        return false;
    }
    
    // Установка параметров
    ads->setGain(currentGain);
    
    // Установка скорости выборки через прямой доступ к регистрам
    setDataRate(dataRate);
    
    // Проверка подключения после инициализации
    if (!checkConnection()) {
        Serial.println("ERROR: ADS1115 connection test failed after initialization!");
        initialized = false;
        return false;
    }
    
    initialized = true;
    
    Serial.print("ADS1115 initialized successfully! ");
    Serial.print("Gain: ");
    Serial.print(getMaxVoltage(), 3);
    Serial.print("V, Data rate: ");
    Serial.print(dataRate);
    Serial.print(" SPS, Resolution: ");
    Serial.print(getResolution());
    Serial.println(" bits");
    
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
        // Не логируем ошибку каждый раз, только при первой попытке
        static bool errorLogged = false;
        if (!errorLogged) {
            Serial.println("ERROR: ADS1115 not initialized!");
            errorLogged = true;
        }
        return 0;
    }
    
    // Валидация каналов
    if (channel1 > 3 || channel2 > 3) {
        static unsigned long lastInvalidChannelTime = 0;
        unsigned long now = millis();
        if (now - lastInvalidChannelTime > 10000) {  // Логируем не чаще раза в 10 секунд
            Serial.print("ERROR: Invalid channel number (must be 0-3)! Got: ");
            Serial.print(channel1);
            Serial.print(", ");
            Serial.println(channel2);
            lastInvalidChannelTime = now;
        }
        return 0;
    }
    
    // Проверка подключения перед чтением
    if (!checkConnection()) {
        static unsigned long lastErrorTime = 0;
        unsigned long now = millis();
        if (now - lastErrorTime > 5000) {  // Логируем ошибку не чаще раза в 5 секунд
            Serial.println("WARNING: ADS1115 connection lost! Attempting to reconnect...");
            // Попытка повторной инициализации
            if (ads->begin(i2cAddress)) {
                Serial.println("ADS1115 reconnected successfully");
                initialized = true;
            } else {
                Serial.println("ADS1115 reconnection failed");
                initialized = false;
                lastErrorTime = now;
                return 0;
            }
            lastErrorTime = now;
        } else {
            return 0;
        }
    }
    
    // Чтение дифференциального значения
    // Библиотека Adafruit использует специальные константы для дифференциальных каналов
    int16_t value = 0;
    bool readSuccess = false;
    
    // Маппинг каналов на дифференциальные пары библиотеки Adafruit
    // ADS1X15_DIFF_P0_N1 = канал 0 и 1
    // ADS1X15_DIFF_P0_N3 = канал 0 и 3
    // ADS1X15_DIFF_P1_N3 = канал 1 и 3
    // ADS1X15_DIFF_P2_N3 = канал 2 и 3
    
    if (channel1 == 0 && channel2 == 1) {
        value = ads->readADC_Differential_0_1();
        readSuccess = true;
    } else if (channel1 == 0 && channel2 == 3) {
        value = ads->readADC_Differential_0_3();
        readSuccess = true;
    } else if (channel1 == 1 && channel2 == 3) {
        value = ads->readADC_Differential_1_3();
        readSuccess = true;
    } else if (channel1 == 2 && channel2 == 3) {
        value = ads->readADC_Differential_2_3();
        readSuccess = true;
    } else {
        // Для других комбинаций используем общий метод
        // Библиотека Adafruit не поддерживает все комбинации напрямую
        // Используем одиночные каналы и вычисляем разность программно
        int16_t val1 = ads->readADC_SingleEnded(channel1);
        int16_t val2 = ads->readADC_SingleEnded(channel2);
        value = val1 - val2;
        readSuccess = true;
    }
    
    // Проверка на валидность значения (диапазон для 16-bit знакового: -32768 до 32767)
    // Но фактически ADS1115 возвращает значения от -32768 до +32767
    if (!readSuccess) {
        static unsigned long lastReadErrorTime = 0;
        unsigned long now = millis();
        if (now - lastReadErrorTime > 10000) {
            Serial.println("WARNING: ADS1115 read failed!");
            lastReadErrorTime = now;
        }
        return 0;
    }
    
    // Проверка на выхождение за диапазон (хотя это не должно происходить)
    if (value < -32768 || value > 32767) {
        static unsigned long lastRangeErrorTime = 0;
        unsigned long now = millis();
        if (now - lastRangeErrorTime > 5000) {
            Serial.print("WARNING: ADS1115 reading out of range: ");
            Serial.println(value);
            lastRangeErrorTime = now;
        }
        // Ограничиваем значение
        value = (value < -32768) ? -32768 : ((value > 32767) ? 32767 : value);
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
    if (!ads || !initialized) {
        Serial.println("WARNING: Cannot set data rate - ADS1115 not initialized");
        return;
    }
    
    // Валидируем скорость выборки
    uint16_t validRates[] = {8, 16, 32, 64, 128, 250, 475, 860};
    bool valid = false;
    for (uint8_t i = 0; i < 8; i++) {
        if (rate == validRates[i]) {
            valid = true;
            break;
        }
    }
    
    if (!valid) {
        Serial.print("WARNING: Invalid data rate ");
        Serial.print(rate);
        Serial.println(" SPS, using closest valid rate");
        // Находим ближайшее значение
        uint16_t closest = validRates[0];
        uint16_t minDiff = abs(rate - validRates[0]);
        for (uint8_t i = 1; i < 8; i++) {
            uint16_t diff = abs(rate - validRates[i]);
            if (diff < minDiff) {
                minDiff = diff;
                closest = validRates[i];
            }
        }
        rate = closest;
    }
    
    dataRate = rate;
    
    // Прямая запись в регистр конфигурации для установки скорости выборки
    // Регистр конфигурации: адрес 0x01
    // Биты 7-5 (DR[2:0]): скорость выборки
    uint16_t configReg = 0;
    
    // Читаем текущее значение регистра
    Wire.beginTransmission(i2cAddress);
    Wire.write(0x01);  // Указатель на регистр конфигурации
    if (Wire.endTransmission() != 0) {
        Serial.println("ERROR: Failed to read ADS1115 config register");
        return;
    }
    
    Wire.requestFrom(i2cAddress, (uint8_t)2);
    if (Wire.available() >= 2) {
        uint16_t configHigh = Wire.read();
        uint16_t configLow = Wire.read();
        configReg = (configHigh << 8) | configLow;
    } else {
        Serial.println("ERROR: Failed to read ADS1115 config register data");
        return;
    }
    
    // Очищаем биты скорости выборки (биты 7-5) и устанавливаем новые
    configReg &= ~(0x07 << 5);  // Очищаем биты 7-5
    
    // Маппинг скорости на биты
    uint8_t drBits = 0;
    switch (rate) {
        case 8:   drBits = 0x00; break;
        case 16:  drBits = 0x01; break;
        case 32:  drBits = 0x02; break;
        case 64:  drBits = 0x03; break;
        case 128: drBits = 0x04; break;
        case 250: drBits = 0x05; break;
        case 475: drBits = 0x06; break;
        case 860: drBits = 0x07; break;
        default:  drBits = 0x04; break;  // По умолчанию 128 SPS
    }
    
    configReg |= (drBits << 5);
    
    // Записываем обновленное значение
    Wire.beginTransmission(i2cAddress);
    Wire.write(0x01);  // Указатель на регистр конфигурации
    Wire.write((configReg >> 8) & 0xFF);  // Старший байт
    Wire.write(configReg & 0xFF);         // Младший байт
    uint8_t error = Wire.endTransmission();
    
    if (error != 0) {
        Serial.print("ERROR: Failed to write ADS1115 config register: ");
        Serial.println(error);
        return;
    }
    
    Serial.print("ADS1115 data rate set to: ");
    Serial.print(rate);
    Serial.println(" SPS");
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

void ADS1115Driver::setPowerDown(bool enable) {
    if (!ads || !initialized) {
        return;
    }
    
    // Читаем текущее значение регистра конфигурации
    Wire.beginTransmission(i2cAddress);
    Wire.write(0x01);  // Указатель на регистр конфигурации
    if (Wire.endTransmission() != 0) {
        return;
    }
    
    Wire.requestFrom(i2cAddress, (uint8_t)2);
    if (Wire.available() < 2) {
        return;
    }
    
    uint16_t configHigh = Wire.read();
    uint16_t configLow = Wire.read();
    uint16_t configReg = (configHigh << 8) | configLow;
    
    // Бит 0 (MODE): 0 = непрерывное преобразование, 1 = однократное (power-down)
    if (enable) {
        configReg |= 0x01;  // Устанавливаем бит MODE = 1
    } else {
        configReg &= ~0x01; // Очищаем бит MODE = 0
    }
    
    // Записываем обновленное значение
    Wire.beginTransmission(i2cAddress);
    Wire.write(0x01);
    Wire.write((configReg >> 8) & 0xFF);
    Wire.write(configReg & 0xFF);
    Wire.endTransmission();
}

float ADS1115Driver::readDifferentialVoltage(uint8_t channel1, uint8_t channel2) {
    int16_t rawValue = readDifferential(channel1, channel2);
    return getVoltage(rawValue);
}

uint16_t ADS1115Driver::convertToUnsigned(int16_t rawValue) const {
    // Преобразуем знаковое 16-битное значение (-32768 до 32767) 
    // в беззнаковое 16-битное (0 до 65535)
    // Смещаем значение на 32768 для получения диапазона 0-65535
    return (uint16_t)(rawValue + 32768);
}
