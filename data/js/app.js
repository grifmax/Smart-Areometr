// === Конфигурация ===
const API_BASE = '';  // Пустая строка для относительных путей
const UPDATE_INTERVAL = 2000;  // Fallback интервал для HTTP polling (если WebSocket недоступен)
const MAX_CHART_POINTS = 50;  // Максимум точек на графике

// === Глобальные переменные ===
let alcoholChart = null;
let temperatureChart = null;
let updateTimer = null;
let ws = null;  // WebSocket соединение
let useWebSocket = true;  // Использовать WebSocket вместо HTTP polling
let chartData = {
    labels: [],
    alcohol: [],
    temperature: []
};

// Делаем графики доступными глобально для обновления темы
window.alcoholChart = null;
window.temperatureChart = null;

// === Инициализация ===
document.addEventListener('DOMContentLoaded', () => {
    console.log('Smart Areometr Web Interface initialized');
    initCharts();
    loadStatus();
    loadMeasurement();
    loadBatteryStatus();  // Загружаем статус батареи
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
    
    // Сохраняем глобально для обновления темы
    window.alcoholChart = alcoholChart;

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
    
    // Сохраняем глобально для обновления темы
    window.temperatureChart = temperatureChart;
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

    // Информация об ADC (ADS1115 или встроенный)
    if (data.adc_type) {
        const adcInfoElement = document.getElementById('adcInfo');
        if (adcInfoElement) {
            if (data.adc_type === 'ads1115') {
                adcInfoElement.textContent = `ADS1115 (${data.adc_resolution || 16}-bit)`;
                adcInfoElement.style.color = '#4CAF50';
                // Показываем напряжение сигнала, если доступно
                if (data.signal_voltage !== undefined) {
                    const voltageInfo = document.getElementById('signalVoltage');
                    if (voltageInfo) {
                        voltageInfo.textContent = `${data.signal_voltage.toFixed(4)}V`;
                        voltageInfo.style.display = 'inline';
                    }
                }
            } else {
                adcInfoElement.textContent = `Встроенный (${data.adc_resolution || 12}-bit)`;
                adcInfoElement.style.color = '#9E9E9E';
            }
        }
    }

    // Фракция
    const fractionItem = document.getElementById('fractionItem');
    const fractionValue = document.getElementById('fractionValue');
    const fractionIcon = document.getElementById('fractionIcon');

    if (data.fraction) {
        if (fractionItem) fractionItem.style.display = 'flex';
        
        const fractionInfo = {
            'unknown': { name: 'Ожидание', icon: '🧪', color: '#9E9E9E' },
            'foreshots': { name: 'Первач', icon: '🔴', color: '#D32F2F' },
            'heads': { name: 'Головы', icon: '🟠', color: '#F57C00' },
            'body': { name: 'Тело', icon: '🟢', color: '#4CAF50' },
            'tails': { name: 'Хвосты', icon: '🟡', color: '#FBC02D' },
            'finished': { name: 'Завершено', icon: '⚫', color: '#757575' }
        };
        
        const info = fractionInfo[data.fraction] || fractionInfo['unknown'];
        if (fractionValue) {
            fractionValue.textContent = info.name;
            fractionValue.style.color = info.color;
        }
        if (fractionIcon) fractionIcon.textContent = info.icon;
        
        // Показываем скорость изменения если есть
        if (data.alcohol_rate !== undefined && data.alcohol_rate !== null) {
            const rateText = data.alcohol_rate >= 0 ? 
                `+${data.alcohol_rate.toFixed(2)}` : 
                data.alcohol_rate.toFixed(2);
            if (fractionValue) {
                fractionValue.textContent = `${info.name} (${rateText} %/мин)`;
            }
        }
    } else {
        if (fractionItem) fractionItem.style.display = 'none';
    }

    // Время обновления
    const lastUpdate = document.getElementById('lastUpdate');
    if (lastUpdate) {
        lastUpdate.textContent = new Date().toLocaleTimeString();
    }
    
    // Информация об АЦП (если есть в данных измерения)
    if (data.adc_type) {
        const adcTypeElement = document.getElementById('adcType');
        if (adcTypeElement) {
            if (data.adc_type === 'ads1115') {
                adcTypeElement.textContent = `ADS1115 (${data.adc_resolution || 16}-bit)`;
                adcTypeElement.style.color = '#4CAF50';
            } else {
                adcTypeElement.textContent = `Встроенный (${data.adc_resolution || 12}-bit)`;
                adcTypeElement.style.color = '#9E9E9E';
            }
        }
    }
}

// === Загрузка статуса батареи ===
async function loadBatteryStatus() {
    try {
        const response = await fetch(`${API_BASE}/api/battery/status`);
        if (!response.ok) return;  // Батарея не доступна
        
        const data = await response.json();
        updateBatteryDisplay(data);
    } catch (error) {
        // Игнорируем ошибки - батарея может быть недоступна
    }
}

// === Обновление отображения батареи ===
function updateBatteryDisplay(data) {
    const batteryWidget = document.getElementById('batteryWidget');
    const batteryPercent = document.getElementById('batteryPercent');
    const batteryVoltage = document.getElementById('batteryVoltage');
    const batteryStatus = document.getElementById('batteryStatus');
    
    if (batteryWidget) {
        batteryWidget.style.display = 'flex';
        
        if (batteryPercent) {
            batteryPercent.textContent = `${data.percent || 0}%`;
            // Обновляем иконку батареи в зависимости от уровня заряда
            const batteryIcon = batteryWidget.querySelector('.battery-icon');
            if (batteryIcon) {
                const percent = data.percent || 0;
                if (percent <= 10) {
                    batteryIcon.textContent = '🔴'; // Критический
                    batteryIcon.style.color = '#F44336';
                } else if (percent <= 20) {
                    batteryIcon.textContent = '🟠'; // Низкий
                    batteryIcon.style.color = '#FF9800';
                } else if (percent <= 50) {
                    batteryIcon.textContent = '🟡'; // Средний
                    batteryIcon.style.color = '#FFC107';
                } else {
                    batteryIcon.textContent = '🟢'; // Нормальный
                    batteryIcon.style.color = '#4CAF50';
                }
            }
        }
        
        if (batteryVoltage) {
            batteryVoltage.textContent = `${(data.voltage || 0).toFixed(2)}V`;
        }
        
        if (batteryStatus) {
            if (data.charging) {
                batteryStatus.textContent = 'Зарядка';
                batteryStatus.style.color = '#2196F3';
            } else if (data.low_battery) {
                batteryStatus.textContent = 'Низкий заряд';
                batteryStatus.style.color = '#FF9800';
            } else if (data.critical) {
                batteryStatus.textContent = 'Критический';
                batteryStatus.style.color = '#F44336';
            } else {
                batteryStatus.textContent = 'Норма';
                batteryStatus.style.color = '#4CAF50';
            }
        }
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
    
    // Обновление информации об АЦП
    const adcTypeElement = document.getElementById('adcType');
    const ads1115InfoItem = document.getElementById('ads1115InfoItem');
    const ads1115StatusElement = document.getElementById('ads1115Status');
    
    if (data.ads1115) {
        const ads1115 = data.ads1115;
        if (ads1115.enabled && ads1115.initialized && ads1115.connected) {
            if (adcTypeElement) {
                adcTypeElement.textContent = `ADS1115 (${ads1115.resolution}-bit)`;
                adcTypeElement.style.color = '#4CAF50';
            }
            if (ads1115InfoItem) ads1115InfoItem.style.display = 'flex';
            if (ads1115StatusElement) {
                ads1115StatusElement.textContent = `Подключен (${ads1115.data_rate} SPS)`;
                ads1115StatusElement.style.color = '#4CAF50';
            }
        } else {
            if (adcTypeElement) {
                adcTypeElement.textContent = 'Встроенный (12-bit)';
                adcTypeElement.style.color = '#9E9E9E';
            }
            if (ads1115InfoItem) ads1115InfoItem.style.display = 'none';
        }
    } else {
        if (adcTypeElement) {
            adcTypeElement.textContent = 'Встроенный (12-bit)';
            adcTypeElement.style.color = '#9E9E9E';
        }
        if (ads1115InfoItem) ads1115InfoItem.style.display = 'none';
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

// === WebSocket подключение ===
function initWebSocket() {
    // Определяем протокол (ws или wss)
    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    const wsUrl = `${protocol}//${window.location.host}/ws`;
    
    console.log('Подключение к WebSocket:', wsUrl);
    
    ws = new WebSocket(wsUrl);
    
    ws.onopen = () => {
        console.log('WebSocket подключен');
        updateConnectionStatus(true);
        // Останавливаем HTTP polling, если он был запущен
        stopAutoUpdate();
    };
    
    ws.onmessage = (event) => {
        try {
            const data = JSON.parse(event.data);
            updateMeasurementDisplay(data);
            updateCharts(data);
        } catch (error) {
            console.error('Ошибка парсинга WebSocket данных:', error);
        }
    };
    
    ws.onerror = (error) => {
        console.error('WebSocket ошибка:', error);
        updateConnectionStatus(false);
        // Fallback на HTTP polling при ошибке
        if (!updateTimer) {
            console.log('Переключение на HTTP polling из-за ошибки WebSocket');
            useWebSocket = false;
            loadMeasurement();
            startAutoUpdate();
        }
    };
    
    ws.onclose = () => {
        console.log('WebSocket отключен, переключение на HTTP polling');
        updateConnectionStatus(false);
        // Переключаемся на HTTP polling
        useWebSocket = false;
        if (!updateTimer) {
            loadMeasurement();
            startAutoUpdate();
        }
        // Пытаемся переподключиться через 5 секунд
        setTimeout(() => {
            if (useWebSocket) {
                initWebSocket();
            }
        }, 5000);
    };
}

// === Автоматическое обновление (HTTP polling fallback) ===
function startAutoUpdate() {
    if (updateTimer) clearInterval(updateTimer);

    updateTimer = setInterval(() => {
        loadMeasurement();
        // Обновляем батарею реже - каждые 30 секунд (15 интервалов по 2 секунды)
        if (!window.batteryUpdateCounter) window.batteryUpdateCounter = 0;
        window.batteryUpdateCounter++;
        if (window.batteryUpdateCounter >= 15) {
            loadBatteryStatus();
            window.batteryUpdateCounter = 0;
        }
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
