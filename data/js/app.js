// === Конфигурация ===
const API_BASE = '';  // Пустая строка для относительных путей
const UPDATE_INTERVAL = 2000;  // Обновление каждые 2 секунды
const MAX_CHART_POINTS = 50;  // Максимум точек на графике

// === Глобальные переменные ===
let alcoholChart = null;
let temperatureChart = null;
let updateTimer = null;
let chartData = {
    labels: [],
    alcohol: [],
    temperature: []
};

// === Инициализация ===
document.addEventListener('DOMContentLoaded', () => {
    console.log('Smart Areometr Web Interface initialized');
    initCharts();
    loadStatus();
    loadMeasurement();
    startAutoUpdate();
});

// === Инициализация графиков ===
function initCharts() {
    const alcoholCtx = document.getElementById('alcoholChart');
    const temperatureCtx = document.getElementById('temperatureChart');

    if (!alcoholCtx || !temperatureCtx) return;

    // График крепости
    alcoholChart = new Chart(alcoholCtx, {
        type: 'line',
        data: {
            labels: chartData.labels,
            datasets: [{
                label: 'Крепость (%)',
                data: chartData.alcohol,
                borderColor: '#2196F3',
                backgroundColor: 'rgba(33, 150, 243, 0.1)',
                borderWidth: 2,
                tension: 0.4,
                fill: true,
                pointRadius: 4,
                pointHoverRadius: 6
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: true,
            plugins: {
                legend: {
                    display: true,
                    position: 'top'
                },
                tooltip: {
                    mode: 'index',
                    intersect: false
                }
            },
            scales: {
                y: {
                    beginAtZero: true,
                    max: 100,
                    ticks: {
                        callback: (value) => value + '%'
                    }
                },
                x: {
                    display: true,
                    title: {
                        display: true,
                        text: 'Время'
                    }
                }
            }
        }
    });

    // График температуры
    temperatureChart = new Chart(temperatureCtx, {
        type: 'line',
        data: {
            labels: chartData.labels,
            datasets: [{
                label: 'Температура (°C)',
                data: chartData.temperature,
                borderColor: '#FF9800',
                backgroundColor: 'rgba(255, 152, 0, 0.1)',
                borderWidth: 2,
                tension: 0.4,
                fill: true,
                pointRadius: 4,
                pointHoverRadius: 6
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: true,
            plugins: {
                legend: {
                    display: true,
                    position: 'top'
                },
                tooltip: {
                    mode: 'index',
                    intersect: false
                }
            },
            scales: {
                y: {
                    ticks: {
                        callback: (value) => value + '°C'
                    }
                },
                x: {
                    display: true,
                    title: {
                        display: true,
                        text: 'Время'
                    }
                }
            }
        }
    });
}

// === Загрузка данных измерений ===
async function loadMeasurement() {
    try {
        const response = await fetch(`${API_BASE}/api/measurement`);
        if (!response.ok) throw new Error('Failed to fetch measurement');

        const data = await response.json();
        updateMeasurementDisplay(data);
        updateCharts(data);
        updateConnectionStatus(true);
    } catch (error) {
        console.error('Error loading measurement:', error);
        updateConnectionStatus(false);
    }
}

// === Обновление отображения измерений ===
function updateMeasurementDisplay(data) {
    // Основное значение
    const alcoholValue = document.getElementById('alcoholValue');
    if (alcoholValue) {
        alcoholValue.textContent = data.alcohol.toFixed(1);
        animateValue(alcoholValue);
    }

    // Температура
    const temperatureValue = document.getElementById('temperatureValue');
    if (temperatureValue) {
        temperatureValue.textContent = data.temperature.toFixed(1);
    }

    // Статус калибровки
    const calibrationStatus = document.getElementById('calibrationStatus');
    if (calibrationStatus) {
        calibrationStatus.textContent = data.calibrated ? 'OK' : 'Не откалиброван';
        calibrationStatus.style.color = data.calibrated ? '#4CAF50' : '#F44336';
    }

    // Время обновления
    const lastUpdate = document.getElementById('lastUpdate');
    if (lastUpdate) {
        lastUpdate.textContent = new Date().toLocaleTimeString();
    }
}

// === Обновление графиков ===
function updateCharts(data) {
    const now = new Date().toLocaleTimeString();

    // Добавляем новые данные
    chartData.labels.push(now);
    chartData.alcohol.push(data.alcohol);
    chartData.temperature.push(data.temperature);

    // Ограничиваем количество точек
    if (chartData.labels.length > MAX_CHART_POINTS) {
        chartData.labels.shift();
        chartData.alcohol.shift();
        chartData.temperature.shift();
    }

    // Обновляем графики
    if (alcoholChart) {
        alcoholChart.update('none');  // Без анимации для производительности
    }
    if (temperatureChart) {
        temperatureChart.update('none');
    }
}

// === Загрузка статуса системы ===
async function loadStatus() {
    try {
        const response = await fetch(`${API_BASE}/api/status`);
        if (!response.ok) throw new Error('Failed to fetch status');

        const data = await response.json();
        updateSystemInfo(data);
    } catch (error) {
        console.error('Error loading status:', error);
    }
}

// === Обновление системной информации ===
function updateSystemInfo(data) {
    const fields = {
        'firmwareVersion': data.firmware,
        'wifiMode': data.wifi_mode,
        'ssid': data.ssid,
        'ipAddress': data.ip
    };

    for (const [id, value] of Object.entries(fields)) {
        const element = document.getElementById(id);
        if (element) element.textContent = value;
    }
}

// === Обновление статуса подключения ===
function updateConnectionStatus(connected) {
    const statusBadge = document.getElementById('statusBadge');
    const connectionStatus = document.getElementById('connectionStatus');

    if (statusBadge && connectionStatus) {
        if (connected) {
            statusBadge.classList.remove('error');
            statusBadge.classList.add('connected');
            connectionStatus.textContent = 'Подключено';
        } else {
            statusBadge.classList.remove('connected');
            statusBadge.classList.add('error');
            connectionStatus.textContent = 'Ошибка';
        }
    }
}

// === Автоматическое обновление ===
function startAutoUpdate() {
    if (updateTimer) clearInterval(updateTimer);

    updateTimer = setInterval(() => {
        loadMeasurement();
    }, UPDATE_INTERVAL);
}

function stopAutoUpdate() {
    if (updateTimer) {
        clearInterval(updateTimer);
        updateTimer = null;
    }
}

// === Действия ===
async function measureNow() {
    // Принудительное измерение
    await loadMeasurement();
    showNotification('Измерение выполнено', 'success');
}

function clearChartData(type) {
    if (type === 'alcohol') {
        chartData.labels = [];
        chartData.alcohol = [];
        if (alcoholChart) alcoholChart.update();
    } else if (type === 'temperature') {
        chartData.labels = [];
        chartData.temperature = [];
        if (temperatureChart) temperatureChart.update();
    }
    showNotification('График очищен', 'success');
}

async function exportData() {
    try {
        const data = {
            timestamp: new Date().toISOString(),
            labels: chartData.labels,
            alcohol: chartData.alcohol,
            temperature: chartData.temperature
        };

        const blob = new Blob([JSON.stringify(data, null, 2)], {
            type: 'application/json'
        });

        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = `areometr-data-${Date.now()}.json`;
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);

        showNotification('Данные экспортированы', 'success');
    } catch (error) {
        console.error('Export error:', error);
        showNotification('Ошибка экспорта', 'error');
    }
}

// === Утилиты ===
function animateValue(element) {
    element.style.transform = 'scale(1.1)';
    setTimeout(() => {
        element.style.transform = 'scale(1)';
    }, 200);
}

function showNotification(message, type = 'info') {
    // Простое уведомление (можно заменить на toast библиотеку)
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
            document.body.removeChild(notification);
        }, 300);
    }, 3000);
}

// Добавляем CSS для анимаций уведомлений
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

    #alcoholValue, #temperatureValue {
        transition: transform 0.2s ease-out;
    }
`;
document.head.appendChild(style);

// === Остановка обновлений при уходе со страницы ===
document.addEventListener('visibilitychange', () => {
    if (document.hidden) {
        stopAutoUpdate();
    } else {
        startAutoUpdate();
        loadMeasurement();
    }
});
