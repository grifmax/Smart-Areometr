// Управление мультисенсорным режимом

let sensorsChart = null;
let sensorsData = {
    labels: [],
    datasets: []
};

// Инициализация при загрузке страницы
document.addEventListener('DOMContentLoaded', function() {
    initSensorsChart();
    updateSensorsStatus();
    setInterval(updateSensorsStatus, 2000); // Обновление каждые 2 секунды
});

// Инициализация графика датчиков
function initSensorsChart() {
    const ctx = document.getElementById('sensorsChart');
    if (!ctx) return;

    sensorsChart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: [],
            datasets: []
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            scales: {
                y: {
                    beginAtZero: true,
                    max: 100,
                    title: {
                        display: true,
                        text: 'Крепость (%)'
                    }
                },
                x: {
                    title: {
                        display: true,
                        text: 'Время'
                    }
                }
            },
            plugins: {
                legend: {
                    display: true,
                    position: 'top'
                }
            }
        }
    });
}

// Обновление статуса датчиков
async function updateSensorsStatus() {
    try {
        const response = await fetch('/api/sensors/status');
        if (response.ok) {
            const data = await response.json();
            updateSensorsDisplay(data);
            updateSensorsChart(data);
            updateSensorsComparison(data);
            updateSensorsControl(data);
        }
    } catch (error) {
        console.error('Ошибка обновления статуса датчиков:', error);
    }
}

// Обновление отображения датчиков
function updateSensorsDisplay(data) {
    const container = document.getElementById('sensorsStatus');
    if (!container || !data.sensors) return;

    container.innerHTML = data.sensors.map(sensor => {
        const isActive = sensor.active;
        const isCalibrated = sensor.calibrated;
        
        return `
            <div class="sensor-card ${isActive ? 'active' : 'inactive'}">
                <div class="sensor-header">
                    <h3>${sensor.name || `Датчик ${sensor.id + 1}`}</h3>
                    ${isActive ? '<span class="badge badge-success">АКТИВЕН</span>' : '<span class="badge badge-secondary">ВЫКЛЮЧЕН</span>'}
                    ${isCalibrated ? '<span class="badge badge-info">КАЛИБРОВАН</span>' : '<span class="badge badge-warning">НЕ КАЛИБРОВАН</span>'}
                </div>
                <div class="sensor-body">
                    <div class="sensor-info">
                        <div class="info-row">
                            <span class="info-label">Крепость:</span>
                            <span class="info-value">${sensor.alcohol.toFixed(1)}%</span>
                        </div>
                        <div class="info-row">
                            <span class="info-label">Температура:</span>
                            <span class="info-value">${sensor.temperature.toFixed(1)}°C</span>
                        </div>
                        <div class="info-row">
                            <span class="info-label">Стабильность:</span>
                            <span class="info-value">${sensor.stability}%</span>
                        </div>
                        <div class="info-row">
                            <span class="info-label">Сырое значение:</span>
                            <span class="info-value">${sensor.raw_value}</span>
                        </div>
                    </div>
                </div>
            </div>
        `;
    }).join('');

    // Обновляем информацию о средних значениях
    if (data.average_alcohol !== undefined) {
        const avgElement = document.getElementById('averageAlcohol');
        if (avgElement) {
            avgElement.textContent = data.average_alcohol.toFixed(1) + '%';
        }
    }

    // Предупреждение об аномалиях
    if (data.anomalies_detected) {
        const anomalyWarning = document.getElementById('anomalyWarning');
        if (anomalyWarning) {
            anomalyWarning.style.display = 'block';
        }
    }
}

// Обновление графика датчиков
function updateSensorsChart(data) {
    if (!sensorsChart || !data.sensors) return;

    const now = new Date().toLocaleTimeString();
    
    // Добавляем новую метку времени
    sensorsData.labels.push(now);
    if (sensorsData.labels.length > 50) {
        sensorsData.labels.shift();
    }

    // Обновляем данные для каждого датчика
    data.sensors.forEach((sensor, index) => {
        if (!sensorsData.datasets[index]) {
            // Создаем новый dataset для датчика
            const colors = ['#2196F3', '#4CAF50', '#FF9800', '#9C27B0'];
            sensorsData.datasets[index] = {
                label: sensor.name || `Датчик ${sensor.id + 1}`,
                data: [],
                borderColor: colors[index % colors.length],
                backgroundColor: colors[index % colors.length] + '40',
                tension: 0.4,
                fill: false
            };
        }

        if (sensor.active) {
            sensorsData.datasets[index].data.push(sensor.alcohol);
        } else {
            sensorsData.datasets[index].data.push(null);
        }

        // Ограничиваем количество точек
        if (sensorsData.datasets[index].data.length > 50) {
            sensorsData.datasets[index].data.shift();
        }
    });

    // Обновляем график
    sensorsChart.data.labels = sensorsData.labels;
    sensorsChart.data.datasets = sensorsData.datasets;
    sensorsChart.update('none');
}

// Обновление сравнения датчиков
function updateSensorsComparison(data) {
    const container = document.getElementById('sensorsComparison');
    if (!container || !data.sensors) return;

    const activeSensors = data.sensors.filter(s => s.active && s.calibrated);
    
    if (activeSensors.length < 2) {
        container.innerHTML = '<p>Для сравнения нужно минимум 2 активных откалиброванных датчика</p>';
        return;
    }

    // Вычисляем расхождения
    const alcoholValues = activeSensors.map(s => s.alcohol);
    const minAlcohol = Math.min(...alcoholValues);
    const maxAlcohol = Math.max(...alcoholValues);
    const deviation = maxAlcohol - minAlcohol;

    container.innerHTML = `
        <div class="comparison-stats">
            <div class="stat-item">
                <div class="stat-label">Минимальная крепость</div>
                <div class="stat-value">${minAlcohol.toFixed(1)}%</div>
            </div>
            <div class="stat-item">
                <div class="stat-label">Максимальная крепость</div>
                <div class="stat-value">${maxAlcohol.toFixed(1)}%</div>
            </div>
            <div class="stat-item">
                <div class="stat-label">Расхождение</div>
                <div class="stat-value ${deviation > 5 ? 'error' : ''}">${deviation.toFixed(1)}%</div>
            </div>
            <div class="stat-item">
                <div class="stat-label">Средняя крепость</div>
                <div class="stat-value">${data.average_alcohol.toFixed(1)}%</div>
            </div>
        </div>
        ${data.anomalies_detected ? '<div class="alert alert-warning">⚠️ Обнаружены аномалии в показаниях датчиков!</div>' : ''}
    `;
}

// Обновление управления датчиками
function updateSensorsControl(data) {
    const container = document.getElementById('sensorsControl');
    if (!container || !data.sensors) return;

    container.innerHTML = data.sensors.map(sensor => {
        return `
            <div class="form-group">
                <label class="checkbox-label">
                    <input type="checkbox" ${sensor.active ? 'checked' : ''} 
                           onchange="toggleSensor(${sensor.id}, this.checked)">
                    <span>${sensor.name || `Датчик ${sensor.id + 1}`}</span>
                </label>
            </div>
        `;
    }).join('');
}

// Переключение датчика
async function toggleSensor(sensorId, enabled) {
    try {
        const response = await fetch('/api/sensors/enable', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                sensor_id: sensorId,
                enabled: enabled
            })
        });

        if (response.ok) {
            // Успешно
            updateSensorsStatus();
        } else {
            alert('Ошибка переключения датчика');
        }
    } catch (error) {
        console.error('Ошибка переключения датчика:', error);
        alert('Ошибка переключения датчика');
    }
}

