// === UI для калькулятора разбавления спирта ===

// Сохранение значений в localStorage
const STORAGE_KEY = 'dilution_calculator_values';

/**
 * Переключение между режимами калькулятора
 */
function switchCalcMode(mode) {
    console.log('Switching to mode:', mode);
    
    // Обновляем вкладки
    const tabs = document.querySelectorAll('.tab-btn');
    tabs.forEach(btn => {
        if (btn) {
            btn.classList.remove('active');
            if (btn.dataset.mode === mode) {
                btn.classList.add('active');
            }
        }
    });

    // Скрываем все формы
    const forms = document.querySelectorAll('.calc-form');
    forms.forEach(form => {
        if (form) {
            form.classList.remove('active');
        }
    });
    
    // Показываем нужную форму в зависимости от режима
    let formId = '';
    if (mode === 'water') {
        formId = 'calcFormWater';
    } else if (mode === 'solution') {
        formId = 'calcFormSolution';
    } else if (mode === 'reverse') {
        formId = 'calcFormReverse';
    }
    
    const activeForm = document.getElementById(formId);
    if (activeForm) {
        activeForm.classList.add('active');
        console.log('Activated form:', formId);
    } else {
        console.error('Form not found:', formId);
        alert('Ошибка: форма не найдена: ' + formId);
    }

    // Скрываем результаты предыдущего режима
    document.querySelectorAll('.calc-results').forEach(results => {
        if (results) results.classList.remove('show');
    });
    document.querySelectorAll('.calc-error').forEach(error => {
        if (error) error.classList.remove('show');
    });

    // Сохраняем выбранный режим
    try {
        saveValues();
    } catch (e) {
        console.warn('Failed to save values:', e);
    }
}

/**
 * Валидация входных данных
 */
function validateInput(input) {
    const value = parseFloat(input.value);
    const min = parseFloat(input.min);
    const max = parseFloat(input.max);

    if (isNaN(value) || value < min || (max && value > max)) {
        input.classList.add('invalid');
        return false;
    } else {
        input.classList.remove('invalid');
        return true;
    }
}

/**
 * Настройка валидации форм
 */
function setupFormValidation() {
    const inputs = document.querySelectorAll('.calc-form input[type="number"]');
    inputs.forEach(input => {
        input.addEventListener('blur', () => {
            validateInput(input);
        });
        input.addEventListener('input', () => {
            if (input.classList.contains('invalid')) {
                validateInput(input);
            }
        });
    });
}

/**
 * Расчет разбавления водой
 */
function calculateWaterDilution() {
    console.log('calculateWaterDilution called');
    const errorDiv = document.getElementById('waterError');
    const resultsDiv = document.getElementById('waterResults');
    
    if (!errorDiv || !resultsDiv) {
        console.error('Error or results div not found');
        return;
    }
    
    // Скрываем предыдущие результаты и ошибки
    errorDiv.classList.remove('show');
    resultsDiv.classList.remove('show');

    // Проверяем доступность функции
    if (typeof diluteWithWater === 'undefined') {
        errorDiv.textContent = 'Ошибка: модуль расчетов не загружен. Проверьте консоль браузера.';
        errorDiv.classList.add('show');
        console.error('diluteWithWater function is not available');
        console.log('Available functions:', Object.keys(window).filter(k => k.includes('dilute') || k.includes('calc')));
        return;
    }

    try {
        // Получаем значения
        const volumeAlcohol = parseFloat(document.getElementById('waterVolumeAlcohol').value);
        const sourceStrength = parseFloat(document.getElementById('waterSourceStrength').value);
        const targetStrength = parseFloat(document.getElementById('waterTargetStrength').value);
        const tempAlcohol = parseFloat(document.getElementById('waterTempAlcohol').value);
        const tempWater = parseFloat(document.getElementById('waterTempWater').value);

        // Валидация
        if (!validateInput(document.getElementById('waterVolumeAlcohol')) ||
            !validateInput(document.getElementById('waterSourceStrength')) ||
            !validateInput(document.getElementById('waterTargetStrength')) ||
            !validateInput(document.getElementById('waterTempAlcohol')) ||
            !validateInput(document.getElementById('waterTempWater'))) {
            throw new Error('Проверьте правильность введенных данных');
        }

        if (targetStrength >= sourceStrength) {
            throw new Error('Целевая крепость должна быть меньше исходной');
        }

        // Выполняем расчет
        const result = diluteWithWater(volumeAlcohol, sourceStrength, targetStrength, tempAlcohol, tempWater);

        // Отображаем результаты
        document.getElementById('waterResultVolumeWater').textContent = result.volumeWater.toFixed(1);
        document.getElementById('waterResultFinalVolume').textContent = result.finalVolume.toFixed(1);
        document.getElementById('waterResultFinalStrength').textContent = result.finalStrength.toFixed(2);
        document.getElementById('waterResultContraction').textContent = 
            `${result.volumeContraction.toFixed(1)} мл (${result.contractionPercent.toFixed(2)}%)`;
        
        const tempCorrection = applyTemperatureCorrection(targetStrength, (tempAlcohol + tempWater) / 2);
        const correctionSign = tempCorrection >= 0 ? '+' : '';
        document.getElementById('waterResultTempCorrection').textContent = 
            `${correctionSign}${tempCorrection.toFixed(2)}%`;

        resultsDiv.classList.add('show');
        saveValues();

    } catch (error) {
        errorDiv.textContent = error.message || 'Ошибка расчета';
        errorDiv.classList.add('show');
    }
}

/**
 * Расчет смешивания двух растворов
 */
function calculateSolutionMix() {
    const errorDiv = document.getElementById('solutionError');
    const resultsDiv = document.getElementById('solutionResults');
    
    errorDiv.classList.remove('show');
    resultsDiv.classList.remove('show');

    // Проверяем доступность функции
    if (typeof mixSolutions === 'undefined') {
        errorDiv.textContent = 'Ошибка: модуль расчетов не загружен';
        errorDiv.classList.add('show');
        console.error('mixSolutions function is not available');
        return;
    }

    try {
        // Получаем значения
        const vol1 = parseFloat(document.getElementById('sol1Volume').value);
        const str1 = parseFloat(document.getElementById('sol1Strength').value);
        const temp1 = parseFloat(document.getElementById('sol1Temp').value);
        const vol2 = parseFloat(document.getElementById('sol2Volume').value);
        const str2 = parseFloat(document.getElementById('sol2Strength').value);
        const temp2 = parseFloat(document.getElementById('sol2Temp').value);

        // Валидация
        const inputs = [
            document.getElementById('sol1Volume'),
            document.getElementById('sol1Strength'),
            document.getElementById('sol1Temp'),
            document.getElementById('sol2Volume'),
            document.getElementById('sol2Strength'),
            document.getElementById('sol2Temp')
        ];

        for (const input of inputs) {
            if (!validateInput(input)) {
                throw new Error('Проверьте правильность введенных данных');
            }
        }

        // Выполняем расчет
        const result = mixSolutions(vol1, str1, temp1, vol2, str2, temp2);

        // Отображаем результаты
        document.getElementById('solutionResultFinalVolume').textContent = result.finalVolume.toFixed(1);
        document.getElementById('solutionResultFinalStrength').textContent = result.finalStrength.toFixed(2);
        document.getElementById('solutionResultContraction').textContent = result.contractionVolume.toFixed(1);
        document.getElementById('solutionResultContractionPercent').textContent = 
            `${result.contractionPercent.toFixed(2)}%`;
        document.getElementById('solutionResultMassEthanol').textContent = 
            `${result.totalMassEthanol.toFixed(2)} г`;

        resultsDiv.classList.add('show');
        saveValues();

    } catch (error) {
        errorDiv.textContent = error.message || 'Ошибка расчета';
        errorDiv.classList.add('show');
    }
}

/**
 * Обратный расчет
 */
function calculateReverse() {
    const errorDiv = document.getElementById('reverseError');
    const resultsDiv = document.getElementById('reverseResults');
    
    errorDiv.classList.remove('show');
    resultsDiv.classList.remove('show');

    // Проверяем доступность функции
    if (typeof reverseCalculation === 'undefined') {
        errorDiv.textContent = 'Ошибка: модуль расчетов не загружен';
        errorDiv.classList.add('show');
        console.error('reverseCalculation function is not available');
        return;
    }

    try {
        // Получаем значения
        const targetVolume = parseFloat(document.getElementById('reverseTargetVolume').value);
        const targetStrength = parseFloat(document.getElementById('reverseTargetStrength').value);
        const sourceStrength = parseFloat(document.getElementById('reverseSourceStrength').value);
        const temp = parseFloat(document.getElementById('reverseTemp').value);

        // Валидация
        if (!validateInput(document.getElementById('reverseTargetVolume')) ||
            !validateInput(document.getElementById('reverseTargetStrength')) ||
            !validateInput(document.getElementById('reverseSourceStrength')) ||
            !validateInput(document.getElementById('reverseTemp'))) {
            throw new Error('Проверьте правильность введенных данных');
        }

        if (targetStrength >= sourceStrength) {
            throw new Error('Целевая крепость должна быть меньше исходной');
        }

        // Выполняем расчет
        const result = reverseCalculation(targetVolume, targetStrength, sourceStrength, temp);

        // Отображаем результаты
        document.getElementById('reverseResultVolumeAlcohol').textContent = result.volumeAlcohol.toFixed(1);
        document.getElementById('reverseResultVolumeWater').textContent = result.volumeWater.toFixed(1);
        document.getElementById('reverseResultContraction').textContent = result.contractionVolume.toFixed(1);
        document.getElementById('reverseResultFinalStrength').textContent = result.finalStrength.toFixed(2);
        document.getElementById('reverseResultContractionPercent').textContent = 
            `${result.contractionPercent.toFixed(2)}%`;

        resultsDiv.classList.add('show');
        saveValues();

    } catch (error) {
        errorDiv.textContent = error.message || 'Ошибка расчета';
        errorDiv.classList.add('show');
    }
}

/**
 * Сохранение значений в localStorage
 */
function saveValues() {
    try {
        const values = {
            water: {
                volumeAlcohol: document.getElementById('waterVolumeAlcohol').value,
                sourceStrength: document.getElementById('waterSourceStrength').value,
                targetStrength: document.getElementById('waterTargetStrength').value,
                tempAlcohol: document.getElementById('waterTempAlcohol').value,
                tempWater: document.getElementById('waterTempWater').value
            },
            solution: {
                vol1: document.getElementById('sol1Volume').value,
                str1: document.getElementById('sol1Strength').value,
                temp1: document.getElementById('sol1Temp').value,
                vol2: document.getElementById('sol2Volume').value,
                str2: document.getElementById('sol2Strength').value,
                temp2: document.getElementById('sol2Temp').value
            },
            reverse: {
                targetVolume: document.getElementById('reverseTargetVolume').value,
                targetStrength: document.getElementById('reverseTargetStrength').value,
                sourceStrength: document.getElementById('reverseSourceStrength').value,
                temp: document.getElementById('reverseTemp').value
            }
        };
        localStorage.setItem(STORAGE_KEY, JSON.stringify(values));
    } catch (error) {
        console.warn('Failed to save calculator values:', error);
    }
}

/**
 * Загрузка сохраненных значений из localStorage
 */
function loadSavedValues() {
    try {
        const saved = localStorage.getItem(STORAGE_KEY);
        if (!saved) return;

        const values = JSON.parse(saved);

        // Загружаем значения для режима "С водой"
        if (values.water) {
            if (document.getElementById('waterVolumeAlcohol')) {
                document.getElementById('waterVolumeAlcohol').value = values.water.volumeAlcohol || 100;
                document.getElementById('waterSourceStrength').value = values.water.sourceStrength || 96;
                document.getElementById('waterTargetStrength').value = values.water.targetStrength || 40;
                document.getElementById('waterTempAlcohol').value = values.water.tempAlcohol || 20;
                document.getElementById('waterTempWater').value = values.water.tempWater || 20;
            }
        }

        // Загружаем значения для режима "Два раствора"
        if (values.solution) {
            if (document.getElementById('sol1Volume')) {
                document.getElementById('sol1Volume').value = values.solution.vol1 || 500;
                document.getElementById('sol1Strength').value = values.solution.str1 || 40;
                document.getElementById('sol1Temp').value = values.solution.temp1 || 20;
                document.getElementById('sol2Volume').value = values.solution.vol2 || 500;
                document.getElementById('sol2Strength').value = values.solution.str2 || 70;
                document.getElementById('sol2Temp').value = values.solution.temp2 || 20;
            }
        }

        // Загружаем значения для режима "Обратный расчет"
        if (values.reverse) {
            if (document.getElementById('reverseTargetVolume')) {
                document.getElementById('reverseTargetVolume').value = values.reverse.targetVolume || 1000;
                document.getElementById('reverseTargetStrength').value = values.reverse.targetStrength || 40;
                document.getElementById('reverseSourceStrength').value = values.reverse.sourceStrength || 96;
                document.getElementById('reverseTemp').value = values.reverse.temp || 20;
            }
        }
    } catch (error) {
        console.warn('Failed to load saved calculator values:', error);
    }
}

// Делаем функции доступными глобально
window.switchCalcMode = switchCalcMode;
window.calculateWaterDilution = calculateWaterDilution;
window.calculateSolutionMix = calculateSolutionMix;
window.calculateReverse = calculateReverse;

// Инициализация при загрузке страницы
document.addEventListener('DOMContentLoaded', () => {
    console.log('Dilution calculator UI initialized');
    
    // Проверяем, что функции доступны
    if (typeof switchCalcMode === 'undefined') {
        console.error('switchCalcMode is not defined!');
    }
    if (typeof calculateWaterDilution === 'undefined') {
        console.error('calculateWaterDilution is not defined!');
    }
    
    // Убеждаемся, что только первая форма видна
    const forms = document.querySelectorAll('.calc-form');
    forms.forEach((form, index) => {
        if (index === 0) {
            form.classList.add('active');
        } else {
            form.classList.remove('active');
        }
    });
    
    loadSavedValues();
    setupFormValidation();
    
    // Автоматически подставляем текущую температуру из датчика
    if (typeof currentTemp !== 'undefined') {
        const tempInputs = document.querySelectorAll('#waterTempAlcohol, #waterTempWater, #reverseTemp, #sol1Temp, #sol2Temp');
        tempInputs.forEach(input => {
            if (input && input.value === '20') {
                input.value = currentTemp.toFixed(1);
            }
        });
    }
    
    // Сохраняем значения при изменении полей
    const inputs = document.querySelectorAll('.calc-form input');
    inputs.forEach(input => {
        input.addEventListener('change', saveValues);
    });
    
    console.log('Calculator ready. Functions available:', {
        switchCalcMode: typeof switchCalcMode,
        calculateWaterDilution: typeof calculateWaterDilution,
        calculateSolutionMix: typeof calculateSolutionMix,
        calculateReverse: typeof calculateReverse
    });
});
