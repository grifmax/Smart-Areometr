#ifndef ADS1115_DRIVER_H
#define ADS1115_DRIVER_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

/**
 * @brief Драйвер для работы с внешним 16-битным АЦП ADS1115
 * 
 * Обеспечивает высокоточное дифференциальное измерение сигнала
 * емкостного датчика с подавлением синфазных помех.
 */
class ADS1115Driver {
private:
    Adafruit_ADS1115* ads;           // Указатель на объект библиотеки Adafruit
    uint8_t i2cAddress;               // Адрес I2C устройства
    adsGain_t currentGain;            // Текущее усиление (PGA)
    uint16_t dataRate;                // Скорость выборки (SPS)
    bool initialized;                 // Флаг инициализации
    
    /**
     * @brief Проверить подключение устройства по I2C
     */
    bool checkConnection();

public:
    /**
     * @brief Конструктор
     * @param address Адрес I2C устройства (по умолчанию 0x48)
     */
    ADS1115Driver(uint8_t address = 0x48);
    
    /**
     * @brief Деструктор
     */
    ~ADS1115Driver();
    
    /**
     * @brief Инициализация драйвера и устройства
     * @return true если инициализация успешна
     */
    bool begin();
    
    /**
     * @brief Проверить, подключено ли устройство
     * @return true если устройство отвечает
     */
    bool isConnected();
    
    /**
     * @brief Прочитать дифференциальное значение между двумя каналами
     * @param channel1 Первый канал (0-3 для AIN0-AIN3)
     * @param channel2 Второй канал (0-3 для AIN0-AIN3)
     * @return 16-битное значение (-32768 до 32767) или 0 при ошибке
     */
    int16_t readDifferential(uint8_t channel1, uint8_t channel2);
    
    /**
     * @brief Прочитать одиночный канал относительно GND
     * @param channel Номер канала (0-3)
     * @return 16-битное значение (0 до 32767) или 0 при ошибке
     */
    int16_t readSingleEnded(uint8_t channel);
    
    /**
     * @brief Установить усиление (PGA)
     * @param gain Усиление: GAIN_ONE, GAIN_TWO, GAIN_FOUR, GAIN_EIGHT
     */
    void setGain(adsGain_t gain);
    
    /**
     * @brief Получить текущее усиление
     * @return Текущее значение усиления
     */
    adsGain_t getGain() const { return currentGain; }
    
    /**
     * @brief Установить скорость выборки
     * @param rate Скорость в SPS: 8, 16, 32, 64, 128, 250, 475, 860
     */
    void setDataRate(uint16_t rate);
    
    /**
     * @brief Получить скорость выборки
     * @return Текущая скорость в SPS
     */
    uint16_t getDataRate() const { return dataRate; }
    
    /**
     * @brief Конвертировать сырое значение в напряжение
     * @param rawValue Сырое 16-битное значение
     * @return Напряжение в вольтах
     */
    float getVoltage(int16_t rawValue);
    
    /**
     * @brief Тест подключения и работоспособности устройства
     * @return true если устройство работает корректно
     */
    bool testConnection();
    
    /**
     * @brief Получить максимальное напряжение для текущего усиления
     * @return Максимальное напряжение в вольтах
     */
    float getMaxVoltage() const;
    
    /**
     * @brief Получить разрешение в битах
     * @return 16 бит для ADS1115
     */
    uint8_t getResolution() const { return 16; }
    
    /**
     * @brief Проверить, инициализирован ли драйвер
     * @return true если драйвер инициализирован
     */
    bool isInitialized() const { return initialized; }
};

#endif // ADS1115_DRIVER_H
