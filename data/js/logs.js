// === Конфигурация ===
const API_BASE = '';

// === Глобальные переменные ===
let logsData = [];

// === Инициализация ===
document.addEventListener('DOMContentLoaded', () => {
    console.log('Logs page initialized');
    refreshLogs();
});

// === Загрузка логов ===
async function refreshLogs() {
    try {
        const response = await fetch(`${API_BASE}/api/logs`);
        if (!response.ok) throw new Error('Failed to fetch logs');

        const data = await response.json();
        logsData = data.measurements || [];

        if (logsData.length === 0) {
            showNoData();
        } else {
            renderLogs();
            updateStatistics();
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
