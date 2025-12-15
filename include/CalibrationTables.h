#ifndef CALIBRATION_TABLES_H
#define CALIBRATION_TABLES_H

#include <Arduino.h>

/**
 * @brief Таблицы для точной калибровки и температурной коррекции
 *
 * Основано на ГОСТ 3639-79 "Растворы водно-спиртовые"
 * и ГОСТ Р 51135-98 "Изделия ликероводочные и водка"
 */

// Максимальное количество калибровочных точек
#define MAX_CALIBRATION_POINTS 10

/**
 * @brief Структура калибровочной точки
 */
struct CalibrationPoint {
    float alcoholPercent;  // Известная крепость (%)
    float rawValue;        // Сырое значение датчика
    float temperature;     // Температура при калибровке (°C)
};

/**
 * @brief Таблица температурной коррекции для водно-спиртовых растворов
 *
 * Показывает, как нужно корректировать показания при отклонении температуры
 * от эталонной (20°C) для разных крепостей
 */
class TemperatureCorrectionTable {
public:
    /**
     * @brief Получить коррекцию крепости в зависимости от температуры
     * @param alcoholPercent Измеренная крепость при текущей температуре (%)
     * @param currentTemp Текущая температура (°C)
     * @param referenceTemp Эталонная температура (°C), обычно 20°C
     * @return Скорректированная крепость (%)
     */
    static float getCorrection(float alcoholPercent, float currentTemp, float referenceTemp = 20.0f);

private:
    // Таблица коррекции: [температура][крепость] -> поправка
    // Основано на данных ГОСТ 3639-79
    static const float correctionTable[21][11];  // 0-40°C (шаг 2°C), 0-100% (шаг 10%)

    /**
     * @brief Билинейная интерполяция для получения точного значения
     */
    static float bilinearInterpolation(float temp, float alcohol);
};

/**
 * @brief Класс для работы с калибровочной кривой
 */
class CalibrationCurve {
private:
    CalibrationPoint points[MAX_CALIBRATION_POINTS];
    uint8_t pointCount;
    bool sorted;

    /**
     * @brief Сортировка точек по крепости
     */
    void sortPoints();

    /**
     * @brief Линейная интерполяция между двумя точками
     */
    float linearInterpolate(float x, float x0, float y0, float x1, float y1);

public:
    CalibrationCurve();

    /**
     * @brief Добавить калибровочную точку
     * @param alcoholPercent Известная крепость (%)
     * @param rawValue Сырое значение датчика
     * @param temperature Температура при калибровке (°C)
     * @return true если точка добавлена успешно
     */
    bool addPoint(float alcoholPercent, float rawValue, float temperature = 20.0f);

    /**
     * @brief Удалить все точки
     */
    void clear();

    /**
     * @brief Получить количество точек
     */
    uint8_t getPointCount() const { return pointCount; }

    /**
     * @brief Получить точку по индексу
     */
    CalibrationPoint getPoint(uint8_t index) const;

    /**
     * @brief Преобразовать сырое значение в крепость
     * @param rawValue Сырое значение датчика
     * @return Крепость в процентах
     */
    float rawToAlcohol(float rawValue);

    /**
     * @brief Преобразовать крепость в сырое значение
     * @param alcoholPercent Крепость (%)
     * @return Ожидаемое сырое значение
     */
    float alcoholToRaw(float alcoholPercent);

    /**
     * @brief Проверка валидности калибровки
     * @return true если калибровка корректна (минимум 2 точки)
     */
    bool isValid() const { return pointCount >= 2; }

    /**
     * @brief Сохранить в JSON
     */
    String toJSON();

    /**
     * @brief Загрузить из JSON
     */
    bool fromJSON(const String& json);
};

/**
 * @brief Стандартные калибровочные растворы
 */
namespace StandardSolutions {
    // Типичные калибровочные точки для самогоноварения
    const float WATER = 0.0f;           // Вода
    const float BEER = 5.0f;            // Пиво
    const float WINE = 12.0f;           // Вино
    const float LIQUOR = 40.0f;         // Водка/самогон
    const float STRONG_LIQUOR = 70.0f;  // Крепкий спирт
    const float PURE_ALCOHOL = 96.0f;   // Чистый спирт

    /**
     * @brief Получить рекомендуемые точки калибровки
     */
    struct RecommendedPoints {
        float alcohol;
        const char* name;
        const char* description;
    };

    const RecommendedPoints RECOMMENDED[] = {
        {0.0f, "Вода", "Дистиллированная или кипяченая вода"},
        {40.0f, "Водка", "Магазинная водка 40%"},
        {70.0f, "Спирт 70%", "Разбавленный медицинский спирт"},
        {96.0f, "Спирт", "Медицинский спирт 96%"}
    };

    const uint8_t RECOMMENDED_COUNT = 4;
}

#endif // CALIBRATION_TABLES_H
