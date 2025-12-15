// === Конфигурация ===
const API_BASE = '';
const UPDATE_INTERVAL = 2000; // 2 секунды

// === Глобальные переменные ===
let updateTimer = null;
let calibrationPoints = [];
let currentRaw = 0;
let currentTemp = 20.0;
let currentStability = 0;

// === Инициализация ===
document.addEventListener('DOMContentLoaded', () => {
    console.log('Multi-point calibration page initialized');
    loadCalibrationData();
    startSensorMonitoring();
});

// === Загрузка данных калибровки ===
async function loadCalibrationData() {
    try {
        // Получаем статус
        const statusResponse = await fetch(`${API_BASE}/api/status`);
        if (statusResponse.ok) {
            const status = await statusResponse.json();
            updateCalibrationStatus(status.calibrated);
        }

        // Получаем калибровочные данные
        const calResponse = await fetch(`${API_BASE}/api/calibration`);
        if (calResponse.ok) {
            const calData = await calResponse.json();
            calibrationPoints = calData.points || [];
            updatePointsList();
            updateCalibrationInfo();
        } else {
            calibrationPoints = [];
            updatePointsList();
        }
    } catch (error) {
        console.error('Error loading calibration data:', error);
        updateCalibrationStatus(false);
    }
}

// === Обновление статуса калибровки ===
function updateCalibrationStatus(isCalibrated) {
    const icon = document.getElementById('statusEmoji');
    const text = document.getElementById('calibrationStatusText');
    const desc = document.getElementById('calibrationStatusDesc');
    const info = document.getElementById('calibrationInfo');

    if (isCalibrated && calibrationPoints.length >= 2) {
        icon.textContent = '✅';
        text.textContent = 'Система откалибрована';
        desc.textContent = `Используется ${calibrationPoints.length}-точечная калибровка`;
        document.getElementById('calibrationIcon').classList.add('success');
        info.style.display = 'block';
    } else if (calibrationPoints.length === 1) {
        icon.textContent = '⚠️';
        text.textContent = 'Недостаточно точек';
        desc.textContent = 'Добавьте минимум 1 точку для калибровки (всего нужно 2+)';
        document.getElementById('calibrationIcon').classList.remove('success');
        info.style.display = 'block';
    } else {
        icon.textContent = '⚠️';
        text.textContent = 'Требуется калибровка';
        desc.textContent = 'Добавьте минимум 2 калибровочные точки';
        document.getElementById('calibrationIcon').classList.remove('success');
        info.style.display = 'none';
    }
}

// === Обновление информации о калибровке ===
function updateCalibrationInfo() {
    if (calibrationPoints.length === 0) {
        return;
    }

    // Находим диапазоны
    let minAlc = 1000, maxAlc = -1;
    let minRaw = 65535, maxRaw = 0;

    calibrationPoints.forEach(point => {
        if (point.alcoholPercent < minAlc) minAlc = point.alcoholPercent;
        if (point.alcoholPercent > maxAlc) maxAlc = point.alcoholPercent;
        if (point.rawValue < minRaw) minRaw = point.rawValue;
        if (point.rawValue > maxRaw) maxRaw = point.rawValue;
    });

    document.getElementById('pointCount').textContent = calibrationPoints.length;
    document.getElementById('alcoholRange').textContent = `${minAlc.toFixed(1)}% - ${maxAlc.toFixed(1)}%`;
    document.getElementById('rawRange').textContent = `${minRaw.toFixed(0)} - ${maxRaw.toFixed(0)}`;
}

// === Мониторинг датчиков ===
function startSensorMonitoring() {
    if (updateTimer) clearInterval(updateTimer);

    updateSensorValues(); // Сразу обновляем
    updateTimer = setInterval(updateSensorValues, UPDATE_INTERVAL);
}

async function updateSensorValues() {
    try {
        const response = await fetch(`${API_BASE}/api/measurement`);
        if (!response.ok) return;

        const data = await response.json();

        // Обновляем текущие значения
        currentTemp = data.temperature || 20.0;
        currentRaw = data.raw_value || 0;
        currentStability = data.stability || 0;

        // Отображаем
        document.getElementById('currentRawValue').textContent = currentRaw.toFixed(0);
        document.getElementById('currentTemp').textContent = currentTemp.toFixed(1) + '°C';

        const stabilityElem = document.getElementById('stability');
        stabilityElem.textContent = currentStability.toFixed(0) + '%';

        // Цвет стабильности
        if (currentStability >= 70) {
            stabilityElem.style.color = '#4CAF50';
        } else if (currentStability >= 50) {
            stabilityElem.style.color = '#FF9800';
        } else {
            stabilityElem.style.color = '#F44336';
        }

        // Автоматически подставляем температуру в форму
        document.getElementById('temperature').value = currentTemp.toFixed(1);

    } catch (error) {
        console.error('Error fetching sensor values:', error);
    }
}

// === Быстрый выбор точки ===
function setQuickPoint(alcohol, name) {
    document.getElementById('knownAlcohol').value = alcohol;
    showNotification(`Выбрана точка: ${name} (${alcohol}%)`, 'info');
}

// === Добавление калибровочной точки ===
async function addCalibrationPoint() {
    const knownAlcohol = parseFloat(document.getElementById('knownAlcohol').value);
    const temperature = parseFloat(document.getElementById('temperature').value);

    // Валидация
    if (isNaN(knownAlcohol) || knownAlcohol < 0 || knownAlcohol > 100) {
        showNotification('Крепость должна быть от 0 до 100%', 'error');
        return;
    }

    if (isNaN(temperature) || temperature < 0 || temperature > 40) {
        showNotification('Температура должна быть от 0 до 40°C', 'error');
        return;
    }

    // Проверка стабильности
    if (currentStability < 50) {
        const confirm = window.confirm(
            `Низкая стабильность сигнала (${currentStability.toFixed(0)}%)!\n\n` +
            'Рекомендуется:\n' +
            '- Убрать пузырьки воздуха\n' +
            '- Подождать дольше\n' +
            '- Очистить датчик\n\n' +
            'Продолжить добавление точки?'
        );
        if (!confirm) return;
    }

    const btn = document.getElementById('addPointBtnText');
    const originalText = btn.textContent;
    btn.textContent = 'Добавление...';

    try {
        const response = await fetch(`${API_BASE}/api/calibration/point`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                alcohol_percent: knownAlcohol,
                temperature: temperature
            })
        });

        if (!response.ok) {
            const error = await response.json();
            throw new Error(error.message || 'Failed to add point');
        }

        const result = await response.json();

        showNotification(
            `Точка добавлена: ${knownAlcohol}% (raw: ${result.raw_value})`,
            'success'
        );

        // Перезагружаем данные
        await loadCalibrationData();

    } catch (error) {
        console.error('Error adding calibration point:', error);
        showNotification('Ошибка добавления точки: ' + error.message, 'error');
    } finally {
        btn.textContent = originalText;
    }
}

// === Удаление точки ===
async function deletePoint(index) {
    if (!confirm('Удалить эту калибровочную точку?')) {
        return;
    }

    try {
        const response = await fetch(`${API_BASE}/api/calibration/point/${index}`, {
            method: 'DELETE'
        });

        if (!response.ok) throw new Error('Failed to delete point');

        showNotification('Точка удалена', 'success');
        await loadCalibrationData();

    } catch (error) {
        console.error('Error deleting point:', error);
        showNotification('Ошибка удаления точки', 'error');
    }
}

// === Очистка всех точек ===
async function clearAllPoints() {
    if (!confirm('Удалить ВСЕ калибровочные точки? Это действие нельзя отменить!')) {
        return;
    }

    try {
        const response = await fetch(`${API_BASE}/api/calibration`, {
            method: 'DELETE'
        });

        if (!response.ok) throw new Error('Failed to clear calibration');

        showNotification('Калибровка очищена', 'success');
        calibrationPoints = [];
        updatePointsList();
        updateCalibrationStatus(false);

    } catch (error) {
        console.error('Error clearing calibration:', error);
        showNotification('Ошибка очистки калибровки', 'error');
    }
}

// === Обновление списка точек ===
function updatePointsList() {
    const container = document.getElementById('pointsList');

    if (calibrationPoints.length === 0) {
        container.innerHTML = `
            <div class="empty-state">
                <p>Калибровочные точки отсутствуют</p>
                <small>Добавьте минимум 2 точки для калибровки</small>
            </div>
        `;
        return;
    }

    // Сортируем по крепости
    const sortedPoints = [...calibrationPoints].sort((a, b) =>
        a.alcoholPercent - b.alcoholPercent
    );

    let html = '<div class="points-grid">';

    sortedPoints.forEach((point, index) => {
        const actualIndex = calibrationPoints.indexOf(point);
        html += `
            <div class="point-card">
                <div class="point-header">
                    <span class="point-number">#${index + 1}</span>
                    <button class="btn-icon-delete" onclick="deletePoint(${actualIndex})" title="Удалить">
                        ×
                    </button>
                </div>
                <div class="point-body">
                    <div class="point-main">
                        <div class="point-alcohol">${point.alcoholPercent.toFixed(1)}%</div>
                        <div class="point-label">Крепость</div>
                    </div>
                    <div class="point-details">
                        <div class="point-detail">
                            <span class="detail-label">Raw:</span>
                            <span class="detail-value">${point.rawValue.toFixed(0)}</span>
                        </div>
                        <div class="point-detail">
                            <span class="detail-label">Temp:</span>
                            <span class="detail-value">${point.temperature.toFixed(1)}°C</span>
                        </div>
                    </div>
                </div>
            </div>
        `;
    });

    html += '</div>';

    // Добавляем график (если есть минимум 2 точки)
    if (calibrationPoints.length >= 2) {
        html += '<div class="calibration-curve-info">';
        html += '<p><strong>✓ Калибровочная кривая построена</strong></p>';
        html += '<small>Используется линейная интерполяция между точками</small>';
        html += '</div>';
    }

    container.innerHTML = html;
}

// === Утилиты ===
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
        max-width: 400px;
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
        startSensorMonitoring();
        loadCalibrationData();
    }
});

// Добавляем анимации
const style = document.createElement('style');
style.textContent = `
    @keyframes slideIn {
        from {
            transform: translateX(400px);
            opacity: 0;
        }
        to {
            transform: translateX(0);
            opacity: 1;
        }
    }

    @keyframes slideOut {
        from {
            transform: translateX(0);
            opacity: 1;
        }
        to {
            transform: translateX(400px);
            opacity: 0;
        }
    }
`;
document.head.appendChild(style);
