// === Конфигурация ===
const API_BASE = '';

// === Глобальные переменные ===
let logsData = [];
let alcoholChart = null;
let temperatureChart = null;
let currentTimeFilter = 'all';

// === Инициализация ===
document.addEventListener('DOMContentLoaded', () => {
    console.log('Logs page initialized');
    initCharts();
    refreshLogs();
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
            labels: [],
            datasets: [{
                label: 'Крепость (%)',
                data: [],
                borderColor: '#2196F3',
                backgroundColor: 'rgba(33, 150, 243, 0.1)',
                borderWidth: 2,
                tension: 0.4,
                fill: true,
                pointRadius: 2,
                pointHoverRadius: 4
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
            labels: [],
            datasets: [{
                label: 'Температура (°C)',
                data: [],
                borderColor: '#FF9800',
                backgroundColor: 'rgba(255, 152, 0, 0.1)',
                borderWidth: 2,
                tension: 0.4,
                fill: true,
                pointRadius: 2,
                pointHoverRadius: 4
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

// === Загрузка логов ===
async function refreshLogs() {
    try {
        const response = await fetch(`${API_BASE}/api/logs`);
        if (!response.ok) throw new Error('Failed to fetch logs');

        const data = await response.json();
        logsData = data.measurements || [];

        if (logsData.length === 0) {
            showNoData();
            updateCharts();
        } else {
            renderLogs();
            updateStatistics();
            updateCharts();
        }
    } catch (error) {
        console.error('Error loading logs:', error);
        showError('Ошибка загрузки логов');
    }
}

// === Отображение логов ===
function renderLogs() {
    const tbody = document.getElementById('logsTableBody');
    const limit = parseInt(document.getElementById('filterLimit').value);
    const sort = document.getElementById('filterSort').value;

    // Сортировка
    let sortedData = [...logsData];
    if (sort === 'desc') {
        sortedData.reverse();
    }

    // Лимит
    if (limit > 0) {
        sortedData = sortedData.slice(0, limit);
    }

    // Очищаем таблицу
    tbody.innerHTML = '';

    // Заполняем таблицу
    sortedData.forEach((record, index) => {
        const row = document.createElement('tr');

        const date = new Date(record.timestamp);
        const dateStr = date.toLocaleDateString();
        const timeStr = date.toLocaleTimeString();

        row.innerHTML = `
            <td>${index + 1}</td>
            <td>${dateStr} ${timeStr}</td>
            <td class="value-cell">${record.alcohol.toFixed(1)}%</td>
            <td class="value-cell">${record.temperature.toFixed(1)}°C</td>
            <td class="center">${record.compensated ? '✓' : '—'}</td>
        `;

        tbody.appendChild(row);
    });

    // Показываем таблицу
    document.getElementById('logsTable').style.display = 'table';
    document.getElementById('noDataMessage').style.display = 'none';
}

// === Обновление статистики ===
function updateStatistics() {
    if (logsData.length === 0) return;

    // Общее количество
    document.getElementById('totalRecords').textContent = logsData.length;

    // Средняя крепость
    const avgAlcohol = logsData.reduce((sum, r) => sum + r.alcohol, 0) / logsData.length;
    document.getElementById('avgAlcohol').textContent = avgAlcohol.toFixed(1) + '%';

    // Мин/макс крепость
    const alcoholValues = logsData.map(r => r.alcohol);
    const minAlcohol = Math.min(...alcoholValues);
    const maxAlcohol = Math.max(...alcoholValues);
    document.getElementById('minMaxAlcohol').textContent =
        `${minAlcohol.toFixed(1)} / ${maxAlcohol.toFixed(1)}%`;

    // Средняя температура
    const avgTemp = logsData.reduce((sum, r) => sum + r.temperature, 0) / logsData.length;
    document.getElementById('avgTemp').textContent = avgTemp.toFixed(1) + '°C';
}

// === Экспорт в CSV ===
function exportLogsCSV() {
    if (logsData.length === 0) {
        showNotification('Нет данных для экспорта', 'error');
        return;
    }

    // Заголовок CSV
    let csv = 'Дата,Время,Крепость (%),Температура (°C),Компенсация\n';

    // Данные
    logsData.forEach(record => {
        const date = new Date(record.timestamp);
        const dateStr = date.toLocaleDateString();
        const timeStr = date.toLocaleTimeString();
        const compensated = record.compensated ? 'Да' : 'Нет';

        csv += `${dateStr},${timeStr},${record.alcohol.toFixed(1)},${record.temperature.toFixed(1)},${compensated}\n`;
    });

    // Скачивание
    downloadFile(csv, `areometr-logs-${Date.now()}.csv`, 'text/csv');
    showNotification('Данные экспортированы в CSV', 'success');
}

// === Экспорт в JSON ===
function exportLogsJSON() {
    if (logsData.length === 0) {
        showNotification('Нет данных для экспорта', 'error');
        return;
    }

    const data = {
        exported_at: new Date().toISOString(),
        total_records: logsData.length,
        measurements: logsData
    };

    const json = JSON.stringify(data, null, 2);
    downloadFile(json, `areometr-logs-${Date.now()}.json`, 'application/json');
    showNotification('Данные экспортированы в JSON', 'success');
}

// === Утилиты ===
function showNoData() {
    document.getElementById('logsTable').style.display = 'none';
    document.getElementById('noDataMessage').style.display = 'block';

    // Обнуляем статистику
    document.getElementById('totalRecords').textContent = '0';
    document.getElementById('avgAlcohol').textContent = '--%';
    document.getElementById('minMaxAlcohol').textContent = '-- / --%';
    document.getElementById('avgTemp').textContent = '--°C';
}

function showError(message) {
    const tbody = document.getElementById('logsTableBody');
    tbody.innerHTML = `
        <tr>
            <td colspan="5" class="text-center error-message">
                ❌ ${message}
            </td>
        </tr>
    `;
}

function downloadFile(content, filename, mimeType) {
    const blob = new Blob([content], { type: mimeType });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = filename;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
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

// === Обновление графиков ===
function updateCharts() {
    if (!alcoholChart || !temperatureChart) return;

    // Фильтруем данные по времени
    let filteredData = filterDataByTime(logsData, currentTimeFilter);
    
    // Сортируем по времени (старые к новым для графика)
    filteredData = [...filteredData].sort((a, b) => {
        const timeA = a.unix_timestamp || a.timestamp;
        const timeB = b.unix_timestamp || b.timestamp;
        return timeA - timeB;
    });

    // Подготавливаем данные для графиков
    const labels = [];
    const alcoholData = [];
    const temperatureData = [];

    filteredData.forEach(record => {
        // Форматируем метку времени
        let timeLabel;
        if (record.unix_timestamp) {
            const date = new Date(record.unix_timestamp * 1000);
            timeLabel = date.toLocaleTimeString('ru-RU', { hour: '2-digit', minute: '2-digit' });
        } else if (record.timestamp) {
            // Используем timestamp как относительное время
            const hours = Math.floor(record.timestamp / 3600000);
            const minutes = Math.floor((record.timestamp % 3600000) / 60000);
            timeLabel = `${hours.toString().padStart(2, '0')}:${minutes.toString().padStart(2, '0')}`;
        } else {
            timeLabel = '';
        }

        labels.push(timeLabel);
        alcoholData.push(record.alcohol);
        temperatureData.push(record.temperature);
    });

    // Обновляем график крепости
    alcoholChart.data.labels = labels;
    alcoholChart.data.datasets[0].data = alcoholData;
    alcoholChart.update('none');

    // Обновляем график температуры
    temperatureChart.data.labels = labels;
    temperatureChart.data.datasets[0].data = temperatureData;
    temperatureChart.update('none');
}

// === Фильтрация данных по времени ===
function filterDataByTime(data, period) {
    if (period === 'all') return data;

    const now = Date.now();
    let cutoffTime;

    switch (period) {
        case '1h':
            cutoffTime = now - (1 * 60 * 60 * 1000);
            break;
        case '6h':
            cutoffTime = now - (6 * 60 * 60 * 1000);
            break;
        case '24h':
            cutoffTime = now - (24 * 60 * 60 * 1000);
            break;
        case '7d':
            cutoffTime = now - (7 * 24 * 60 * 60 * 1000);
            break;
        default:
            return data;
    }

    return data.filter(record => {
        let recordTime;
        if (record.unix_timestamp) {
            recordTime = record.unix_timestamp * 1000; // Конвертируем в миллисекунды
        } else if (record.timestamp) {
            // Для millis() timestamp нужно использовать относительное время
            // Предполагаем, что timestamp - это время от старта, а не абсолютное
            // Для точной фильтрации нужен Unix timestamp
            return true; // Показываем все, если нет Unix timestamp
        } else {
            return false;
        }
        return recordTime >= cutoffTime;
    });
}

// === Применение фильтра времени ===
function applyTimeFilter() {
    currentTimeFilter = document.getElementById('filterPeriod').value;
    updateCharts();
}

// === Очистка графиков ===
function clearCharts() {
    if (alcoholChart) {
        alcoholChart.data.labels = [];
        alcoholChart.data.datasets[0].data = [];
        alcoholChart.update();
    }
    if (temperatureChart) {
        temperatureChart.data.labels = [];
        temperatureChart.data.datasets[0].data = [];
        temperatureChart.update();
    }
    showNotification('Графики очищены', 'info');
}
