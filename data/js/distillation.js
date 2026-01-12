// === Конфигурация ===
const API_BASE = '';
const UPDATE_INTERVAL = 2000; // 2 секунды

// === Глобальные переменные ===
let updateTimer = null;
let sessionState = 'idle';

// === Инициализация ===
document.addEventListener('DOMContentLoaded', () => {
    console.log('Distillation session page initialized');
    loadSessionStatus();
    startMonitoring();
});

// === Загрузка статуса сессии ===
async function loadSessionStatus() {
    try {
        const response = await fetch(`${API_BASE}/api/session/status`);
        if (!response.ok) {
            updateSessionDisplay('idle', null);
            return;
        }

        const status = await response.json();
        sessionState = status.state || 'idle';
        updateSessionDisplay(sessionState, status);

    } catch (error) {
        console.error('Error loading session status:', error);
        updateSessionDisplay('idle', null);
    }
}

// === Обновление отображения сессии ===
function updateSessionDisplay(state, data) {
    const statusIndicator = document.getElementById('statusIndicator');
    const statusText = document.getElementById('statusText');
    const statusDetails = document.getElementById('statusDetails');
    const sessionForm = document.getElementById('sessionForm');
    const sessionActions = document.getElementById('sessionActions');
    const startBtn = document.getElementById('startBtn');
    const pauseBtn = document.getElementById('pauseBtn');
    const stopBtn = document.getElementById('stopBtn');
    const exportJSONBtn = document.getElementById('exportJSONBtn');
    const exportCSVBtn = document.getElementById('exportCSVBtn');

    // Обновляем индикатор статуса
    if (statusIndicator) {
        statusIndicator.className = 'status-indicator';
        if (state === 'running') {
            statusIndicator.classList.add('status-running');
            if (statusText) statusText.textContent = 'В процессе';
        } else if (state === 'paused') {
            statusIndicator.classList.add('status-paused');
            if (statusText) statusText.textContent = 'На паузе';
        } else if (state === 'finished') {
            statusIndicator.classList.add('status-finished');
            if (statusText) statusText.textContent = 'Завершена';
        } else {
            statusIndicator.classList.add('status-idle');
            if (statusText) statusText.textContent = 'Не начата';
        }
    }

    // Обновляем детали
    if (statusDetails && data) {
        let details = '';
        if (data.duration_formatted) {
            details += `Длительность: ${data.duration_formatted}`;
        }
        if (data.data_points !== undefined) {
            details += ` | Точек: ${data.data_points}`;
        }
        if (data.progress !== undefined) {
            details += ` | Прогресс: ${data.progress.toFixed(1)}%`;
        }
        statusDetails.textContent = details;
    }

    // Показываем/скрываем элементы управления
    if (state === 'idle') {
        if (sessionForm) sessionForm.style.display = 'block';
        if (sessionActions) sessionActions.style.display = 'none';
        if (startBtn) startBtn.disabled = false;
        if (exportJSONBtn) exportJSONBtn.disabled = true;
        if (exportCSVBtn) exportCSVBtn.disabled = true;
        const exportPDFBtn = document.getElementById('exportPDFBtn');
        if (exportPDFBtn) exportPDFBtn.disabled = true;
    } else {
        if (sessionForm) sessionForm.style.display = 'none';
        if (sessionActions) sessionActions.style.display = 'flex';
        if (startBtn) startBtn.disabled = true;
        if (exportJSONBtn) exportJSONBtn.disabled = false;
        if (exportCSVBtn) exportCSVBtn.disabled = false;
    }

    // Обновляем кнопку паузы
    if (pauseBtn) {
        pauseBtn.textContent = state === 'paused' ? 'Продолжить' : 'Пауза';
    }

    // Обновляем статистику
    if (data) {
        updateStats(data);
    }
}

// === Обновление статистики ===
function updateStats(data) {
    const duration = document.getElementById('duration');
    const dataPoints = document.getElementById('dataPoints');
    const progress = document.getElementById('progress');
    const collected = document.getElementById('collected');

    if (duration && data.duration_formatted) {
        duration.textContent = data.duration_formatted;
    }

    if (dataPoints && data.data_points !== undefined) {
        dataPoints.textContent = data.data_points;
    }

    if (progress && data.progress !== undefined) {
        progress.textContent = data.progress.toFixed(1) + '%';
    }

    if (collected && data.collected !== undefined) {
        collected.textContent = data.collected.toFixed(2) + ' л';
    }
}

// === Начать сессию ===
async function startSession() {
    const name = document.getElementById('sessionName').value || 'Сессия';
    const mashVol = parseFloat(document.getElementById('mashVolume').value) || 0;
    const expectedYield = parseFloat(document.getElementById('expectedYield').value) || 0;

    try {
        const response = await fetch(`${API_BASE}/api/session/start`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                name: name,
                mash_volume: mashVol
            })
        });

        if (!response.ok) throw new Error('Failed to start session');

        showNotification('Сессия начата!', 'success');
        loadSessionStatus();

    } catch (error) {
        console.error('Error starting session:', error);
        showNotification('Ошибка начала сессии', 'error');
    }
}

// === Пауза/продолжить ===
async function pauseSession() {
    try {
        const response = await fetch(`${API_BASE}/api/session/pause`, {
            method: 'POST'
        });

        if (!response.ok) throw new Error('Failed to pause/resume');

        showNotification('Сессия ' + (sessionState === 'paused' ? 'продолжена' : 'приостановлена'), 'success');
        loadSessionStatus();

    } catch (error) {
        console.error('Error pausing session:', error);
        showNotification('Ошибка паузы сессии', 'error');
    }
}

// === Остановить сессию ===
async function stopSession() {
    if (!confirm('Остановить сессию? Данные будут сохранены.')) {
        return;
    }

    try {
        const response = await fetch(`${API_BASE}/api/session/stop`, {
            method: 'POST'
        });

        if (!response.ok) throw new Error('Failed to stop session');

        showNotification('Сессия остановлена и сохранена', 'success');
        loadSessionStatus();

    } catch (error) {
        console.error('Error stopping session:', error);
        showNotification('Ошибка остановки сессии', 'error');
    }
}

// === Экспорт JSON ===
async function exportJSON() {
    try {
        const response = await fetch(`${API_BASE}/api/session/export/json`);
        if (!response.ok) throw new Error('Failed to export');

        const json = await response.json();
        const blob = new Blob([JSON.stringify(json, null, 2)], {
            type: 'application/json'
        });

        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = `session-${Date.now()}.json`;
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);

        showNotification('Данные экспортированы в JSON', 'success');

    } catch (error) {
        console.error('Error exporting JSON:', error);
        showNotification('Ошибка экспорта JSON', 'error');
    }
}

// === Экспорт CSV ===
// === Экспорт в PDF ===
async function exportPDF() {
    try {
        const response = await fetch(`${API_BASE}/api/session/export/json`);
        if (!response.ok) throw new Error('Failed to export');
        
        const sessionData = await response.json();
        
        if (!sessionData || !sessionData.data_points || sessionData.data_points.length === 0) {
            alert('Нет данных сессии для экспорта');
            return;
        }
        
        // Создаем PDF документ
        const { jsPDF } = window.jspdf;
        const doc = new jsPDF();
        
        // Заголовок
        doc.setFontSize(18);
        doc.text('Отчет по сессии дистилляции', 14, 20);
        
        // Информация о сессии
        doc.setFontSize(10);
        let yPos = 30;
        doc.text(`Название: ${sessionData.name || 'Не указано'}`, 14, yPos);
        yPos += 6;
        doc.text(`ID сессии: ${sessionData.session_id || 'N/A'}`, 14, yPos);
        yPos += 6;
        doc.text(`Объем браги: ${sessionData.mash_volume || 0} л`, 14, yPos);
        yPos += 6;
        doc.text(`Длительность: ${sessionData.duration_formatted || 'N/A'}`, 14, yPos);
        yPos += 6;
        doc.text(`Точек данных: ${sessionData.data_points.length}`, 14, yPos);
        yPos += 6;
        doc.text(`Дата создания: ${new Date().toLocaleString('ru-RU')}`, 14, yPos);
        
        // Статистика по фракциям
        if (sessionData.fractions && sessionData.fractions.length > 0) {
            yPos += 10;
            doc.setFontSize(12);
            doc.text('Статистика по фракциям:', 14, yPos);
            yPos += 8;
            doc.setFontSize(10);
            
            sessionData.fractions.forEach(fraction => {
                if (yPos > 270) {
                    doc.addPage();
                    yPos = 20;
                }
                doc.text(`${fraction.name || 'Неизвестно'}:`, 20, yPos);
                doc.text(`Объем: ${(fraction.volume || 0).toFixed(1)} мл`, 80, yPos);
                doc.text(`Средняя крепость: ${(fraction.avg_alcohol || 0).toFixed(1)}%`, 130, yPos);
                yPos += 6;
            });
        }
        
        // Таблица данных (первые 40 записей)
        yPos += 10;
        doc.setFontSize(10);
        doc.text('Данные измерений:', 14, yPos);
        yPos += 8;
        
        // Заголовки таблицы
        doc.setFontSize(9);
        doc.text('Время', 14, yPos);
        doc.text('Крепость', 60, yPos);
        doc.text('Температура', 100, yPos);
        doc.text('Фракция', 150, yPos);
        yPos += 5;
        
        // Линия под заголовками
        doc.line(14, yPos, 190, yPos);
        yPos += 6;
        
        // Данные (максимум 40 записей)
        const recordsToShow = Math.min(40, sessionData.data_points.length);
        for (let i = sessionData.data_points.length - recordsToShow; i < sessionData.data_points.length; i++) {
            if (yPos > 270) {
                doc.addPage();
                yPos = 20;
            }
            
            const point = sessionData.data_points[i];
            const date = new Date(point.timestamp);
            const timeStr = date.toLocaleTimeString('ru-RU');
            
            doc.text(timeStr, 14, yPos);
            doc.text(`${(point.alcohol || 0).toFixed(1)}%`, 60, yPos);
            doc.text(`${(point.temperature || 0).toFixed(1)}°C`, 100, yPos);
            doc.text(point.fraction || 'N/A', 150, yPos);
            yPos += 6;
        }
        
        // Сохраняем PDF
        const fileName = `session_${sessionData.session_id || Date.now()}_${new Date().toISOString().split('T')[0]}.pdf`;
        doc.save(fileName);
        
        showNotification('PDF экспортирован успешно', 'success');
    } catch (error) {
        console.error('Ошибка экспорта в PDF:', error);
        showNotification('Ошибка экспорта в PDF', 'error');
    }
}

async function exportCSV() {
    try {
        const response = await fetch(`${API_BASE}/api/session/export/csv`);
        if (!response.ok) throw new Error('Failed to export');

        const csv = await response.text();
        const blob = new Blob([csv], {
            type: 'text/csv'
        });

        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = `session-${Date.now()}.csv`;
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);

        showNotification('Данные экспортированы в CSV', 'success');

    } catch (error) {
        console.error('Error exporting CSV:', error);
        showNotification('Ошибка экспорта CSV', 'error');
    }
}

// === Мониторинг ===
function startMonitoring() {
    if (updateTimer) clearInterval(updateTimer);

    updateTimer = setInterval(() => {
        loadSessionStatus();
    }, UPDATE_INTERVAL);
}

function stopMonitoring() {
    if (updateTimer) {
        clearInterval(updateTimer);
        updateTimer = null;
    }
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

// === Очистка ===
document.addEventListener('visibilitychange', () => {
    if (document.hidden) {
        stopMonitoring();
    } else {
        startMonitoring();
        loadSessionStatus();
    }
});

// Добавляем стили
const style = document.createElement('style');
style.textContent = `
    .session-controls {
        display: flex;
        flex-direction: column;
        gap: var(--spacing);
    }

    .session-status {
        display: flex;
        align-items: center;
        gap: var(--spacing);
        padding: var(--spacing);
        background: var(--bg-color);
        border-radius: var(--radius);
        border-left: 4px solid var(--border-color);
    }

    .status-indicator {
        width: 16px;
        height: 16px;
        border-radius: 50%;
        flex-shrink: 0;
    }

    .status-indicator.status-idle {
        background: #9E9E9E;
    }

    .status-indicator.status-running {
        background: #4CAF50;
        animation: pulse 2s infinite;
    }

    .status-indicator.status-paused {
        background: #FF9800;
    }

    .status-indicator.status-finished {
        background: #757575;
    }

    .status-text {
        flex: 1;
        display: flex;
        flex-direction: column;
        gap: 4px;
    }

    .status-text strong {
        font-size: 1.125rem;
    }

    .status-text span {
        font-size: 0.875rem;
        color: var(--text-secondary);
    }

    .session-form {
        display: flex;
        flex-direction: column;
        gap: var(--spacing);
    }

    .form-group {
        display: flex;
        flex-direction: column;
        gap: 8px;
    }

    .form-group label {
        font-weight: 500;
        color: var(--text-primary);
    }

    .form-group input {
        padding: 12px;
        border: 2px solid var(--border-color);
        border-radius: var(--radius);
        font-size: 1rem;
        transition: border-color 0.2s;
    }

    .form-group input:focus {
        outline: none;
        border-color: var(--primary-color);
    }

    .session-actions {
        display: flex;
        gap: var(--spacing);
    }

    .btn-primary, .btn-secondary, .btn-danger {
        padding: 12px 24px;
        border: none;
        border-radius: var(--radius);
        font-size: 1rem;
        font-weight: 600;
        cursor: pointer;
        transition: all 0.2s;
    }

    .btn-primary {
        background: var(--primary-color);
        color: white;
    }

    .btn-primary:hover:not(:disabled) {
        background: var(--primary-dark);
    }

    .btn-primary:disabled {
        opacity: 0.5;
        cursor: not-allowed;
    }

    .btn-secondary {
        background: #FF9800;
        color: white;
    }

    .btn-secondary:hover {
        background: #F57C00;
    }

    .btn-danger {
        background: #F44336;
        color: white;
    }

    .btn-danger:hover {
        background: #D32F2F;
    }

    .export-actions {
        display: flex;
        gap: var(--spacing);
    }

    .export-actions button {
        display: flex;
        align-items: center;
        gap: 8px;
    }

    @keyframes pulse {
        0%, 100% {
            opacity: 1;
        }
        50% {
            opacity: 0.5;
        }
    }

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
