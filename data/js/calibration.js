// === Конфигурация ===
const API_BASE = '';
const UPDATE_INTERVAL = 1000;

// === Глобальные переменные ===
let updateTimer = null;
let calibrationStep = 0;  // 0 - не начата, 1 - вода, 2 - спирт
let waterCalibrated = false;

// === Инициализация ===
document.addEventListener('DOMContentLoaded', () => {
    console.log('Calibration page initialized');
    loadCalibrationStatus();
    startRawValueMonitoring();
});

// === Загрузка статуса калибровки ===
async function loadCalibrationStatus() {
    try {
        const response = await fetch(`${API_BASE}/api/status`);
        if (!response.ok) throw new Error('Failed to fetch status');

        const data = await response.json();
        updateCalibrationStatus(data.calibrated);

        // Попытка получить калибровочные значения (если API поддерживает)
        try {
            const calResponse = await fetch(`${API_BASE}/api/calibration`);
            if (calResponse.ok) {
                const calData = await calResponse.json();
                updateCalibrationValues(calData);
            }
        } catch (e) {
            console.log('Calibration values not available');
        }
    } catch (error) {
        console.error('Error loading calibration status:', error);
        updateCalibrationStatus(false);
    }
}

// === Обновление статуса калибровки ===
function updateCalibrationStatus(isCalibrated) {
    const icon = document.getElementById('statusEmoji');
    const text = document.getElementById('calibrationStatusText');
    const desc = document.getElementById('calibrationStatusDesc');

    if (isCalibrated) {
        icon.textContent = '✅';
        text.textContent = 'Система откалибрована';
        desc.textContent = 'Устройство готово к работе';
        document.getElementById('calibrationIcon').classList.add('success');
    } else {
        icon.textContent = '⚠️';
        text.textContent = 'Требуется калибровка';
        desc.textContent = 'Выполните двухточечную калибровку для точных измерений';
        document.getElementById('calibrationIcon').classList.remove('success');
    }
}

// === Обновление калибровочных значений ===
function updateCalibrationValues(data) {
    const waterInput = document.getElementById('waterValue');
    const alcoholInput = document.getElementById('alcoholValue');
    const tempRefInput = document.getElementById('tempRef');
    const tempCoeffInput = document.getElementById('tempCoeff');

    if (waterInput && data.water_value) {
        waterInput.value = data.water_value.toFixed(1);
    }
    if (alcoholInput && data.alcohol_value) {
        alcoholInput.value = data.alcohol_value.toFixed(1);
    }
    if (tempRefInput && data.temperature_reference) {
        tempRefInput.value = data.temperature_reference.toFixed(1);
    }
    if (tempCoeffInput && data.temperature_coefficient) {
        tempCoeffInput.value = data.temperature_coefficient.toFixed(2);
    }
}

// === Мониторинг сырых значений ===
function startRawValueMonitoring() {
    if (updateTimer) clearInterval(updateTimer);

    updateTimer = setInterval(async () => {
        try {
            // Получаем сырое значение датчика
            const response = await fetch(`${API_BASE}/api/measurement`);
            if (!response.ok) return;

            const data = await response.json();

            // Обновляем отображение (используем timestamp как сырое значение для демонстрации)
            // В реальности нужен отдельный endpoint для raw values
            const rawValue = Math.floor(Math.random() * 100 + 50); // Заглушка

            document.getElementById('waterRawValue').textContent = rawValue;
            document.getElementById('alcoholRawValue').textContent = rawValue;
        } catch (error) {
            console.error('Error fetching raw values:', error);
        }
    }, UPDATE_INTERVAL);
}

// === Калибровка на воде ===
async function calibrateWater() {
    const btn = document.getElementById('calibrateWaterBtn');
    const step1 = document.getElementById('step1');

    try {
        // Отключаем кнопку
        btn.disabled = true;
        btn.textContent = 'Калибровка...';

        // Показываем прогресс
        showProgress('Калибровка на воде...', 50);

        // Отправляем запрос на калибровку
        const response = await fetch(`${API_BASE}/api/calibrate/water`, {
            method: 'POST'
        });

        if (!response.ok) throw new Error('Calibration failed');

        const data = await response.json();
        console.log('Water calibration response:', data);

        // Успех
        showProgress('Калибровка на воде завершена!', 100);

        setTimeout(() => {
            hideProgress();
            step1.classList.add('completed');
            waterCalibrated = true;

            // Активируем второй шаг
            document.getElementById('step2').classList.add('active');
            document.getElementById('calibrateAlcoholBtn').disabled = false;

            btn.textContent = '✓ Вода откалибрована';
            btn.classList.remove('btn-primary');
            btn.classList.add('btn-success');

            showNotification('Калибровка на воде выполнена!', 'success');
        }, 1000);

    } catch (error) {
        console.error('Calibration error:', error);
        btn.disabled = false;
        btn.textContent = 'Калибровать на воде';
        hideProgress();
        showNotification('Ошибка калибровки на воде', 'error');
    }
}

// === Калибровка на спирте ===
async function calibrateAlcohol() {
    if (!waterCalibrated) {
        showNotification('Сначала выполните калибровку на воде!', 'error');
        return;
    }

    const btn = document.getElementById('calibrateAlcoholBtn');
    const step2 = document.getElementById('step2');

    try {
        btn.disabled = true;
        btn.textContent = 'Калибровка...';

        showProgress('Калибровка на спирте...', 75);

        const response = await fetch(`${API_BASE}/api/calibrate/alcohol`, {
            method: 'POST'
        });

        if (!response.ok) throw new Error('Calibration failed');

        const data = await response.json();
        console.log('Alcohol calibration response:', data);

        showProgress('Калибровка завершена!', 100);

        setTimeout(() => {
            hideProgress();
            step2.classList.add('completed');

            btn.textContent = '✓ Спирт откалиброван';
            btn.classList.remove('btn-primary');
            btn.classList.add('btn-success');

            // Обновляем общий статус
            updateCalibrationStatus(true);

            showNotification('Калибровка успешно завершена!', 'success');

            // Перезагружаем данные
            loadCalibrationStatus();
        }, 1000);

    } catch (error) {
        console.error('Calibration error:', error);
        btn.disabled = false;
        btn.textContent = 'Калибровать на спирте';
        hideProgress();
        showNotification('Ошибка калибровки на спирте', 'error');
    }
}

// === Сохранение расширенных настроек ===
async function saveAdvancedSettings() {
    const tempRef = document.getElementById('tempRef').value;
    const tempCoeff = document.getElementById('tempCoeff').value;

    try {
        // В реальности здесь нужен endpoint для сохранения настроек
        const response = await fetch(`${API_BASE}/api/settings`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                temperature_reference: parseFloat(tempRef),
                temperature_coefficient: parseFloat(tempCoeff)
            })
        });

        if (response.ok) {
            showNotification('Настройки сохранены!', 'success');
        } else {
            throw new Error('Failed to save settings');
        }
    } catch (error) {
        console.error('Error saving settings:', error);
        showNotification('Ошибка сохранения настроек', 'error');
    }
}

// === Утилиты ===
function showProgress(text, percent) {
    const progress = document.getElementById('calibrationProgress');
    const fill = document.getElementById('progressFill');
    const progressText = document.getElementById('progressText');

    progress.style.display = 'block';
    fill.style.width = percent + '%';
    progressText.textContent = text;
}

function hideProgress() {
    const progress = document.getElementById('calibrationProgress');
    progress.style.display = 'none';
}

function showNotification(message, type = 'info') {
    const notification = document.createElement('div');
    notification.textContent = message;
    notification.style.cssText = `
        position: fixed;
        top: 20px;
        right: 20px;
        padding: 16px 24px;
        background: ${type === 'success' ? '#4CAF50' : type === 'error' ? '#F44336' : '#2196F3'};
        color: white;
        border-radius: 8px;
        box-shadow: 0 4px 8px rgba(0,0,0,0.2);
        z-index: 1000;
        animation: slideIn 0.3s ease-out;
    `;

    document.body.appendChild(notification);

    setTimeout(() => {
        notification.style.animation = 'slideOut 0.3s ease-out';
        setTimeout(() => {
            if (notification.parentNode) {
                document.body.removeChild(notification);
            }
        }, 300);
    }, 3000);
}

// === Очистка при уходе со страницы ===
document.addEventListener('visibilitychange', () => {
    if (document.hidden) {
        if (updateTimer) clearInterval(updateTimer);
    } else {
        startRawValueMonitoring();
        loadCalibrationStatus();
    }
});
