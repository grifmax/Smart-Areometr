// === Профессиональный калькулятор разбавления спирта ===
// Учитывает объемную контракцию и температурные поправки

// Таблица контракции по крепости (%)
const CONTRACTION_TABLE = {
    0: 0.0,
    10: 0.8,
    20: 1.5,
    30: 2.2,
    40: 2.7,
    50: 3.2,
    60: 3.1,
    70: 2.5,
    80: 1.7,
    90: 1.0,
    100: 0.0
};

// Эталонная температура (ГОСТ)
const REFERENCE_TEMP = 20.0;

/**
 * Получить коэффициент контракции для заданной крепости
 * @param {number} alcoholPercent - Крепость в процентах (0-100)
 * @returns {number} Коэффициент контракции в процентах
 */
function getContractionFactor(alcoholPercent) {
    if (alcoholPercent <= 0) return 0.0;
    if (alcoholPercent >= 100) return 0.0;

    // Линейная интерполяция между точками таблицы
    const keys = Object.keys(CONTRACTION_TABLE).map(k => parseFloat(k)).sort((a, b) => a - b);
    
    // Находим ближайшие точки
    let lower = 0, upper = 100;
    for (let i = 0; i < keys.length - 1; i++) {
        if (alcoholPercent >= keys[i] && alcoholPercent <= keys[i + 1]) {
            lower = keys[i];
            upper = keys[i + 1];
            break;
        }
    }

    const lowerFactor = CONTRACTION_TABLE[lower];
    const upperFactor = CONTRACTION_TABLE[upper];
    
    // Линейная интерполяция
    const ratio = (alcoholPercent - lower) / (upper - lower);
    return lowerFactor + (upperFactor - lowerFactor) * ratio;
}

/**
 * Рассчитать плотность этанола при заданной температуре
 * Формула: ρ = 0.80615 - 1.497×10⁻³T + 6.35×10⁻⁶T²
 * @param {number} temp - Температура в °C
 * @returns {number} Плотность в г/мл
 */
function calculateDensityEthanol(temp) {
    const T = temp;
    return 0.80615 - 1.497e-3 * T + 6.35e-6 * T * T;
}

/**
 * Рассчитать плотность воды при заданной температуре
 * Полиномиальная формула для диапазона 0-50°C
 * @param {number} temp - Температура в °C
 * @returns {number} Плотность в г/мл
 */
function calculateDensityWater(temp) {
    const T = temp;
    // Формула для воды (приближенная для 0-50°C)
    // Максимальная плотность при 4°C = 1.000 г/мл
    if (T <= 4) {
        return 1.0 - 0.00005 * (4 - T) * (4 - T);
    } else {
        return 1.0 - 0.0002 * (T - 4) - 0.000006 * (T - 4) * (T - 4);
    }
}

/**
 * Применить температурную поправку по ГОСТ 3639-79
 * Упрощенная модель для диапазона 10-30°C
 * @param {number} alcoholPercent - Крепость в процентах
 * @param {number} temp - Температура в °C
 * @returns {number} Поправка в процентах (положительная = нужно вычитать)
 */
function applyTemperatureCorrection(alcoholPercent, temp) {
    if (temp === REFERENCE_TEMP) return 0.0;
    
    // Упрощенная модель поправки
    // При температуре выше 20°C показания занижены (нужно прибавлять)
    // При температуре ниже 20°C показания завышены (нужно вычитать)
    const deltaT = temp - REFERENCE_TEMP;
    
    // Коэффициент зависит от крепости (больше для средних крепостей)
    let coefficient = 0.0;
    if (alcoholPercent < 20) {
        coefficient = 0.08;
    } else if (alcoholPercent < 40) {
        coefficient = 0.10;
    } else if (alcoholPercent < 60) {
        coefficient = 0.12;
    } else if (alcoholPercent < 80) {
        coefficient = 0.10;
    } else {
        coefficient = 0.08;
    }
    
    return deltaT * coefficient;
}

/**
 * Разбавление спирта водой с учетом контракции и температуры
 * @param {number} volumeAlcohol - Объем исходного спирта (мл)
 * @param {number} sourceStrength - Крепость исходного спирта (%)
 * @param {number} targetStrength - Целевая крепость (%)
 * @param {number} tempAlcohol - Температура спирта (°C, по умолчанию 20)
 * @param {number} tempWater - Температура воды (°C, по умолчанию 20)
 * @returns {Object} Результаты расчета
 */
function diluteWithWater(volumeAlcohol, sourceStrength, targetStrength, tempAlcohol = 20, tempWater = 20) {
    // Валидация
    if (targetStrength >= sourceStrength) {
        throw new Error('Целевая крепость должна быть меньше исходной');
    }
    if (sourceStrength <= 0 || targetStrength < 0) {
        throw new Error('Крепость должна быть положительной');
    }
    if (volumeAlcohol <= 0) {
        throw new Error('Объем спирта должен быть положительным');
    }

    // Рассчитываем массу спирта (этанола) в исходном растворе
    // Плотность водно-спиртового раствора зависит от крепости и температуры
    // Упрощение: используем среднюю плотность
    const densityEthanol = calculateDensityEthanol(tempAlcohol);
    const densityWater = calculateDensityWater(tempWater);
    
    // Масса этанола в исходном растворе
    // Приближенно: масса_этанола = объем × крепость × плотность_этанола
    const massEthanol = volumeAlcohol * (sourceStrength / 100) * densityEthanol;
    
    // Целевой объем раствора (без учета контракции)
    // Из пропорции: масса_этанола / целевой_объем = целевая_крепость / 100
    const targetVolumeNoContraction = (massEthanol / (targetStrength / 100)) / densityEthanol;
    
    // Применяем контракцию
    const contractionFactor = getContractionFactor(targetStrength);
    const contractionVolume = targetVolumeNoContraction * (contractionFactor / 100);
    const finalVolume = targetVolumeNoContraction - contractionVolume;
    
    // Объем воды для добавления
    const volumeWater = finalVolume - volumeAlcohol;
    
    // Финальная крепость с учетом температурных поправок
    const tempCorrection = applyTemperatureCorrection(targetStrength, (tempAlcohol + tempWater) / 2);
    const finalStrength = targetStrength + tempCorrection;
    
    return {
        volumeWater: Math.max(0, volumeWater),
        volumeContraction: contractionVolume,
        finalVolume: finalVolume,
        finalStrength: Math.max(0, Math.min(100, finalStrength)),
        contractionPercent: contractionFactor
    };
}

/**
 * Смешивание двух водно-спиртовых растворов
 * @param {number} vol1 - Объем первого раствора (мл)
 * @param {number} str1 - Крепость первого раствора (%)
 * @param {number} temp1 - Температура первого раствора (°C)
 * @param {number} vol2 - Объем второго раствора (мл)
 * @param {number} str2 - Крепость второго раствора (%)
 * @param {number} temp2 - Температура второго раствора (°C)
 * @returns {Object} Результаты расчета
 */
function mixSolutions(vol1, str1, temp1, vol2, str2, temp2) {
    // Валидация
    if (vol1 <= 0 || vol2 <= 0) {
        throw new Error('Объемы должны быть положительными');
    }
    if (str1 < 0 || str1 > 100 || str2 < 0 || str2 > 100) {
        throw new Error('Крепость должна быть от 0 до 100%');
    }

    // Рассчитываем общую массу этанола
    const densityEth1 = calculateDensityEthanol(temp1);
    const densityEth2 = calculateDensityEthanol(temp2);
    
    const massEth1 = vol1 * (str1 / 100) * densityEth1;
    const massEth2 = vol2 * (str2 / 100) * densityEth2;
    const totalMassEthanol = massEth1 + massEth2;
    
    // Общий объем без контракции
    const totalVolumeNoContraction = vol1 + vol2;
    
    // Средняя крепость без контракции
    const avgStrengthNoContraction = (totalMassEthanol / totalVolumeNoContraction) * 100 / densityEth1;
    
    // Применяем контракцию
    const contractionFactor = getContractionFactor(avgStrengthNoContraction);
    const contractionVolume = totalVolumeNoContraction * (contractionFactor / 100);
    const finalVolume = totalVolumeNoContraction - contractionVolume;
    
    // Финальная крепость с учетом контракции
    const finalStrength = (totalMassEthanol / finalVolume) * 100 / densityEth1;
    
    // Температурная поправка
    const avgTemp = (temp1 + temp2) / 2;
    const tempCorrection = applyTemperatureCorrection(finalStrength, avgTemp);
    const finalStrengthCorrected = finalStrength + tempCorrection;
    
    return {
        finalVolume: finalVolume,
        finalStrength: Math.max(0, Math.min(100, finalStrengthCorrected)),
        contractionVolume: contractionVolume,
        contractionPercent: contractionFactor,
        totalMassEthanol: totalMassEthanol
    };
}

/**
 * Обратный расчет: сколько нужно спирта и воды для получения заданного объема и крепости
 * @param {number} targetVolume - Целевой объем раствора (мл)
 * @param {number} targetStrength - Целевая крепость (%)
 * @param {number} sourceStrength - Крепость исходного спирта (%)
 * @param {number} temp - Температура компонентов (°C, по умолчанию 20)
 * @returns {Object} Результаты расчета
 */
function reverseCalculation(targetVolume, targetStrength, sourceStrength, temp = 20) {
    // Валидация
    if (targetStrength >= sourceStrength) {
        throw new Error('Целевая крепость должна быть меньше исходной');
    }
    if (targetVolume <= 0) {
        throw new Error('Целевой объем должен быть положительным');
    }
    if (sourceStrength <= 0 || targetStrength < 0) {
        throw new Error('Крепость должна быть положительной');
    }

    // Рассчитываем коэффициент контракции для целевой крепости
    const contractionFactor = getContractionFactor(targetStrength);
    
    // Объем до контракции (который нужно получить при смешивании)
    // После контракции этот объем уменьшится до targetVolume
    const volumeBeforeContraction = targetVolume / (1 - contractionFactor / 100);
    
    // Рассчитываем необходимую массу этанола в финальном растворе
    const densityEthanol = calculateDensityEthanol(temp);
    const requiredMassEthanol = targetVolume * (targetStrength / 100) * densityEthanol;
    
    // Объем спирта для получения этой массы этанола
    const volumeAlcohol = requiredMassEthanol / ((sourceStrength / 100) * densityEthanol);
    
    // Объем воды (с учетом того, что общий объем до контракции = volumeBeforeContraction)
    const volumeWater = volumeBeforeContraction - volumeAlcohol;
    
    // Объем контракции
    const contractionVolume = volumeBeforeContraction - targetVolume;
    
    // Температурная поправка для финальной крепости
    const tempCorrection = applyTemperatureCorrection(targetStrength, temp);
    const finalStrength = targetStrength + tempCorrection;
    
    return {
        volumeAlcohol: Math.max(0, volumeAlcohol),
        volumeWater: Math.max(0, volumeWater),
        contractionVolume: contractionVolume,
        finalStrength: Math.max(0, Math.min(100, finalStrength)),
        contractionPercent: contractionFactor
    };
}

// Экспорт функций для использования в других модулях
if (typeof module !== 'undefined' && module.exports) {
    module.exports = {
        getContractionFactor,
        calculateDensityEthanol,
        calculateDensityWater,
        applyTemperatureCorrection,
        diluteWithWater,
        mixSolutions,
        reverseCalculation
    };
}

// Делаем функции доступными глобально для использования в HTML
if (typeof window !== 'undefined') {
    window.getContractionFactor = getContractionFactor;
    window.calculateDensityEthanol = calculateDensityEthanol;
    window.calculateDensityWater = calculateDensityWater;
    window.applyTemperatureCorrection = applyTemperatureCorrection;
    window.diluteWithWater = diluteWithWater;
    window.mixSolutions = mixSolutions;
    window.reverseCalculation = reverseCalculation;
}
