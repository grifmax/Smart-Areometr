// === Конфигурация ===
const API_BASE = '';
const UPDATE_INTERVAL = 2000; // 2 секунды

// === Глобальные переменные ===
let updateTimer = null;
let currentFraction = 'unknown';
let lastAlcoholRate = 0;
let audioEnabled = true;
let audioThreshold = 1.0; // %/мин
let audioContext = null;
let lastAlertTime = 0;

// Данные фракций
const FRACTION_INFO = {
    unknown: { name: 'Ожидание', icon: '🧪', color: '#9E9E9E', desc: 'Начните перегонку' },
    foreshots: { name: 'Первач', icon: '🔴', color: '#D32F2F', desc: 'ОПАСНО! Метанол' },
    heads: { name: 'Головы', icon: '🟠', color: '#F57C00', desc: 'Ацетон, эфиры' },
    body: { name: 'Тело', icon: '🟢', color: '#4CAF50', desc: 'Питьевая фракция' },
    tails: { name: 'Хвосты', icon: '🟡', color: '#FBC02D', desc: 'Сивушные масла' },
    finished: { name: 'Завершено', icon: '⚫', color: '#757575', desc: 'Отбор завершен' }
};

// === Инициализация ===
document.addEventListener('DOMContentLoaded', () => {
    console.log('Fractions page initialized');
    loadMode();
    loadThresholds();
    loadFractionStatus();
    loadStats();
    startMonitoring();
    initAudio();
});

// === Загрузка режима работы ===
async function loadMode() {
    try {
        const response = await fetch(`${API_BASE}/api/fractions/mode`);
        if (!response.ok) {
            console.log('Using default mode');
            return;
        }

        const data = await response.json();
        const mode = data.mode || 'mash';

        // Устанавливаем радио-кнопку
        const radio = document.querySelector(`input[name="detectionMode"][value="${mode}"]`);
        if (radio) radio.checked = true;

        // Загружаем настройки аудио из localStorage
        const savedAudioEnabled = localStorage.getItem('audioEnabled');
        const savedAudioThreshold = localStorage.getItem('audioThreshold');

        if (savedAudioEnabled !== null) {
            audioEnabled = savedAudioEnabled === 'true';
            document.getElementById('audioEnabled').checked = audioEnabled;
        }

        if (savedAudioThreshold !== null) {
            audioThreshold = parseFloat(savedAudioThreshold);
            document.getElementById('audioThreshold').value = audioThreshold;
        }

    } catch (error) {
        console.error('Error loading mode:', error);
    }
}

// === Сохранение режима работы ===
async function saveMode() {
    const selectedMode = document.querySelector('input[name="detectionMode"]:checked').value;
    audioEnabled = document.getElementById('audioEnabled').checked;
    audioThreshold = parseFloat(document.getElementById('audioThreshold').value);

    try {
        // Сохраняем режим на сервер
        const response = await fetch(`${API_BASE}/api/fractions/mode`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ mode: selectedMode })
        });

        if (!response.ok) throw new Error('Failed to save mode');

        // Сохраняем аудио настройки в localStorage
        localStorage.setItem('audioEnabled', audioEnabled);
        localStorage.setItem('audioThreshold', audioThreshold);

        showNotification('Настройки сохранены успешно!', 'success');

    } catch (error) {
        console.error('Error saving mode:', error);
        showNotification('Ошибка сохранения настроек', 'error');
    }
}

// === Инициализация аудио ===
function initAudio() {
    // Создаем AudioContext при первом взаимодействии пользователя
    document.addEventListener('click', () => {
        if (!audioContext) {
            audioContext = new (window.AudioContext || window.webkitAudioContext)();
            console.log('AudioContext initialized');
        }
    }, { once: true });
}

// === Воспроизведение звукового сигнала ===
function playAlertSound() {
    if (!audioContext || !audioEnabled) return;

    // Защита от частых срабатываний (минимум 10 секунд между сигналами)
    const now = Date.now();
    if (now - lastAlertTime < 10000) return;
    lastAlertTime = now;

    try {
        // Создаем осциллятор для генерации звука
        const oscillator = audioContext.createOscillator();
        const gainNode = audioContext.createGain();

        oscillator.connect(gainNode);
        gainNode.connect(audioContext.destination);

        // Настройка звука (три коротких бипа)
        oscillator.frequency.value = 800; // Частота 800 Hz
        oscillator.type = 'sine';

        // Envelope для более приятного звука
        gainNode.gain.setValueAtTime(0, audioContext.currentTime);
        gainNode.gain.linearRampToValueAtTime(0.3, audioContext.currentTime + 0.01);
        gainNode.gain.linearRampToValueAtTime(0, audioContext.currentTime + 0.1);

        oscillator.start(audioContext.currentTime);
        oscillator.stop(audioContext.currentTime + 0.1);

        // Второй бип
        setTimeout(() => {
            const osc2 = audioContext.createOscillator();
            const gain2 = audioContext.createGain();
            osc2.connect(gain2);
            gain2.connect(audioContext.destination);
            osc2.frequency.value = 800;
            osc2.type = 'sine';
            gain2.gain.setValueAtTime(0, audioContext.currentTime);
            gain2.gain.linearRampToValueAtTime(0.3, audioContext.currentTime + 0.01);
            gain2.gain.linearRampToValueAtTime(0, audioContext.currentTime + 0.1);
            osc2.start(audioContext.currentTime);
            osc2.stop(audioContext.currentTime + 0.1);
        }, 150);

        // Третий бип
        setTimeout(() => {
            const osc3 = audioContext.createOscillator();
            const gain3 = audioContext.createGain();
            osc3.connect(gain3);
            gain3.connect(audioContext.destination);
            osc3.frequency.value = 800;
            osc3.type = 'sine';
            gain3.gain.setValueAtTime(0, audioContext.currentTime);
            gain3.gain.linearRampToValueAtTime(0.3, audioContext.currentTime + 0.01);
            gain3.gain.linearRampToValueAtTime(0, audioContext.currentTime + 0.1);
            osc3.start(audioContext.currentTime);
            osc3.stop(audioContext.currentTime + 0.1);
        }, 300);

        console.log('Alert sound played');
    } catch (error) {
        console.error('Error playing sound:', error);
    }
}

// === Загрузка порогов ===
async function loadThresholds() {
    try {
        const response = await fetch(`${API_BASE}/api/fractions/thresholds`);
        if (!response.ok) {
            console.log('Using default thresholds');
            return;
        }

        const thresholds = await response.json();

        document.getElementById('foreshotsVolume').value = thresholds.foreshots_volume || 50;
        document.getElementById('headsThreshold').value = thresholds.heads_threshold || 85;
        document.getElementById('bodyMin').value = thresholds.body_min || 78;
        document.getElementById('bodyMax').value = thresholds.body_max || 85;
        document.getElementById('tailsThreshold').value = thresholds.tails_threshold || 78;
        document.getElementById('rateThreshold').value = thresholds.rate_threshold || 0.5;

    } catch (error) {
        console.error('Error loading thresholds:', error);
    }
}

// === Сохранение порогов ===
async function saveThresholds(event) {
    event.preventDefault();

    const thresholds = {
        foreshots_volume: parseFloat(document.getElementById('foreshotsVolume').value),
        heads_threshold: parseFloat(document.getElementById('headsThreshold').value),
        body_min: parseFloat(document.getElementById('bodyMin').value),
        body_max: parseFloat(document.getElementById('bodyMax').value),
        tails_threshold: parseFloat(document.getElementById('tailsThreshold').value),
        rate_threshold: parseFloat(document.getElementById('rateThreshold').value)
    };

    // Валидация
    if (thresholds.body_min >= thresholds.body_max) {
        showNotification('Ошибка: минимум тела должен быть меньше максимума', 'error');
        return;
    }

    try {
        const response = await fetch(`${API_BASE}/api/fractions/thresholds`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify(thresholds)
        });

        if (!response.ok) throw new Error('Failed to save thresholds');

        showNotification('Пороги сохранены успешно!', 'success');

    } catch (error) {
        console.error('Error saving thresholds:', error);
        showNotification('Ошибка сохранения порогов', 'error');
    }
}

// === Загрузка статуса фракции ===
async function loadFractionStatus() {
    try {
        const response = await fetch(`${API_BASE}/api/fractions/status`);
        if (!response.ok) return;

        const status = await response.json();

        // Обновляем текущую фракцию
        if (status.current_fraction) {
            updateFractionDisplay(status.current_fraction);
        }

        // Обновляем статы
        if (status.alcohol !== undefined) {
            document.getElementById('currentAlcohol').textContent = status.alcohol.toFixed(1) + '%';
        }

        if (status.alcohol_rate !== undefined) {
            const rate = status.alcohol_rate;
            const rateElem = document.getElementById('alcoholRate');
            rateElem.textContent = rate.toFixed(2) + ' %/мин';
            rateElem.style.color = rate < 0 ? '#F44336' : '#4CAF50';

            // Проверка на резкое падение крепости
            if (rate < -audioThreshold && audioEnabled) {
                playAlertSound();
                showNotification(`⚠️ Резкое падение крепости: ${rate.toFixed(2)} %/мин`, 'error');
            }

            lastAlcoholRate = rate;
        }

        if (status.fraction_volume !== undefined) {
            document.getElementById('fractionVolume').textContent =
                Math.round(status.fraction_volume) + ' мл';
        }

        if (status.total_volume !== undefined) {
            document.getElementById('totalVolume').textContent =
                Math.round(status.total_volume) + ' мл';
        }

    } catch (error) {
        console.error('Error loading fraction status:', error);
    }
}

// === Обновление отображения фракции ===
function updateFractionDisplay(fraction) {
    if (currentFraction === fraction) return;

    currentFraction = fraction;
    const info = FRACTION_INFO[fraction] || FRACTION_INFO.unknown;

    const display = document.getElementById('fractionDisplay');
    display.style.borderColor = info.color;
    display.style.backgroundColor = info.color + '20';  // 20 = прозрачность

    document.getElementById('fractionIcon').textContent = info.icon;
    document.getElementById('fractionName').textContent = info.name;
    document.getElementById('fractionName').style.color = info.color;
    document.getElementById('fractionDesc').textContent = info.desc;

    // Анимация смены
    display.classList.add('fraction-change');
    setTimeout(() => display.classList.remove('fraction-change'), 500);

    // Звуковое уведомление (опционально)
    if (fraction !== 'unknown' && fraction !== currentFraction) {
        showNotification(`Смена фракции: ${info.name}`, 'info');
    }
}

// === Загрузка статистики ===
async function loadStats() {
    try {
        const response = await fetch(`${API_BASE}/api/fractions/stats`);
        if (!response.ok) return;

        const data = await response.json();

        if (!data.fractions || data.fractions.length === 0) {
            return; // Пустая статистика
        }

        const container = document.getElementById('statsContainer');
        let html = '<div class="stats-grid">';

        data.fractions.forEach(frac => {
            const info = FRACTION_INFO[frac.type] || FRACTION_INFO.unknown;
            const durationMin = Math.floor(frac.duration / 60);
            const durationSec = frac.duration % 60;

            html += `
                <div class="stat-card" style="border-left: 4px solid ${info.color}">
                    <div class="stat-card-header">
                        <span class="stat-icon">${info.icon}</span>
                        <span class="stat-name">${info.name}</span>
                    </div>
                    <div class="stat-card-body">
                        <div class="stat-row">
                            <span>Длительность:</span>
                            <strong>${durationMin}:${String(durationSec).padStart(2, '0')}</strong>
                        </div>
                        <div class="stat-row">
                            <span>Объем:</span>
                            <strong>${frac.volume} мл</strong>
                        </div>
                        <div class="stat-row">
                            <span>Средняя крепость:</span>
                            <strong>${frac.avg_alcohol}%</strong>
                        </div>
                        <div class="stat-row">
                            <span>Диапазон:</span>
                            <strong>${frac.min_alcohol}% - ${frac.max_alcohol}%</strong>
                        </div>
                    </div>
                </div>
            `;
        });

        html += '</div>';

        // Добавляем общую инфу
        if (data.total_volume) {
            html += `
                <div class="total-stats">
                    <h3>Итого собрано: ${Math.round(data.total_volume)} мл</h3>
                </div>
            `;
        }

        container.innerHTML = html;

    } catch (error) {
        console.error('Error loading stats:', error);
    }
}

// === Сброс сессии ===
async function resetSession() {
    if (!confirm('Начать новую сессию? Текущая статистика будет сброшена.')) {
        return;
    }

    try {
        const response = await fetch(`${API_BASE}/api/fractions/reset`, {
            method: 'POST'
        });

        if (!response.ok) throw new Error('Failed to reset');

        showNotification('Сессия сброшена. Начата новая перегонка.', 'success');

        // Обновляем интерфейс
        setTimeout(() => {
            loadFractionStatus();
            loadStats();
        }, 500);

    } catch (error) {
        console.error('Error resetting session:', error);
        showNotification('Ошибка сброса сессии', 'error');
    }
}

// === Загрузка дефолтных порогов ===
function loadDefaults() {
    if (!confirm('Сбросить пороги к рекомендуемым значениям?')) {
        return;
    }

    document.getElementById('foreshotsVolume').value = 50;
    document.getElementById('headsThreshold').value = 85;
    document.getElementById('bodyMin').value = 78;
    document.getElementById('bodyMax').value = 85;
    document.getElementById('tailsThreshold').value = 78;
    document.getElementById('rateThreshold').value = 0.5;

    showNotification('Установлены рекомендуемые пороги', 'info');
}

// === Мониторинг ===
function startMonitoring() {
    if (updateTimer) clearInterval(updateTimer);

    updateTimer = setInterval(() => {
        loadFractionStatus();
        loadStats();
    }, UPDATE_INTERVAL);
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
        if (updateTimer) clearInterval(updateTimer);
    } else {
        startMonitoring();
        loadFractionStatus();
        loadStats();
    }
});

// Добавляем стили
const style = document.createElement('style');
style.textContent = `
    .current-fraction {
        padding: var(--spacing);
    }

    .fraction-display {
        text-align: center;
        padding: calc(var(--spacing) * 3);
        border: 3px solid var(--border-color);
        border-radius: var(--radius);
        margin-bottom: calc(var(--spacing) * 2);
        transition: all 0.3s;
    }

    .fraction-display.fraction-change {
        transform: scale(1.05);
    }

    .fraction-icon {
        font-size: 5rem;
        margin-bottom: var(--spacing);
    }

    .fraction-name {
        font-size: 2rem;
        font-weight: bold;
        margin-bottom: 8px;
    }

    .fraction-desc {
        font-size: 1.125rem;
        color: var(--text-secondary);
    }

    .fraction-stats {
        display: grid;
        grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
        gap: var(--spacing);
    }

    .stat-box {
        padding: var(--spacing);
        background: var(--bg-color);
        border-radius: var(--radius);
        text-align: center;
    }

    .stat-label {
        display: block;
        font-size: 0.875rem;
        color: var(--text-secondary);
        margin-bottom: 8px;
    }

    .stat-value {
        display: block;
        font-size: 1.5rem;
        font-weight: bold;
        color: var(--primary-color);
    }

    .threshold-section {
        margin: calc(var(--spacing) * 2) 0;
        padding: var(--spacing);
        border-left: 4px solid;
        border-radius: var(--radius);
        background: var(--bg-color);
    }

    .threshold-section.foreshots { border-color: #D32F2F; }
    .threshold-section.heads { border-color: #F57C00; }
    .threshold-section.body { border-color: #4CAF50; }
    .threshold-section.tails { border-color: #FBC02D; }

    .threshold-section h3 {
        margin: 0 0 8px 0;
    }

    .threshold-desc {
        margin: 0 0 var(--spacing) 0;
        color: var(--text-secondary);
        font-size: 0.875rem;
    }

    .stats-grid {
        display: grid;
        grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
        gap: var(--spacing);
    }

    .stat-card {
        padding: var(--spacing);
        background: var(--bg-color);
        border-radius: var(--radius);
    }

    .stat-card-header {
        display: flex;
        align-items: center;
        gap: 8px;
        margin-bottom: var(--spacing);
        font-size: 1.125rem;
        font-weight: bold;
    }

    .stat-icon {
        font-size: 1.5rem;
    }

    .stat-card-body {
        font-size: 0.875rem;
    }

    .stat-row {
        display: flex;
        justify-content: space-between;
        padding: 8px 0;
        border-bottom: 1px solid var(--border-color);
    }

    .stat-row:last-child {
        border-bottom: none;
    }

    .total-stats {
        margin-top: calc(var(--spacing) * 2);
        text-align: center;
        padding: var(--spacing);
        background: var(--primary-light);
        border-radius: var(--radius);
    }

    .total-stats h3 {
        margin: 0;
        color: var(--primary-color);
    }

    .mqtt-integration h3, .mqtt-integration h4 {
        margin-top: calc(var(--spacing) * 1.5);
        margin-bottom: var(--spacing);
        color: var(--primary-color);
    }

    .mqtt-integration pre {
        background: var(--bg-color);
        padding: var(--spacing);
        border-radius: var(--radius);
        overflow-x: auto;
        font-size: 0.875rem;
    }

    .mqtt-integration ul {
        margin-left: 20px;
    }

    .mqtt-integration li {
        margin-bottom: 8px;
    }

    .reference h3, .reference h4 {
        margin-top: calc(var(--spacing) * 1.5);
        margin-bottom: var(--spacing);
        color: var(--primary-color);
    }

    .fraction-info {
        margin: var(--spacing) 0;
        padding: var(--spacing);
        background: var(--bg-color);
        border-radius: var(--radius);
    }

    .fraction-info h4 {
        margin-top: 0;
    }

    .fraction-info p {
        margin: 8px 0;
        font-size: 0.875rem;
    }

    @keyframes slideIn {
        from { transform: translateX(400px); opacity: 0; }
        to { transform: translateX(0); opacity: 1; }
    }

    @keyframes slideOut {
        from { transform: translateX(0); opacity: 1; }
        to { transform: translateX(400px); opacity: 0; }
    }

    .mode-selector h3 {
        margin-top: 0;
        margin-bottom: var(--spacing);
        color: var(--primary-color);
    }

    .radio-group {
        display: flex;
        flex-direction: column;
        gap: var(--spacing);
    }

    .radio-label {
        display: flex;
        align-items: flex-start;
        gap: 12px;
        padding: var(--spacing);
        background: var(--bg-color);
        border: 2px solid var(--border-color);
        border-radius: var(--radius);
        cursor: pointer;
        transition: all 0.2s;
    }

    .radio-label:hover {
        border-color: var(--primary-color);
        background: var(--primary-light);
    }

    .radio-label input[type="radio"] {
        margin-top: 4px;
        cursor: pointer;
    }

    .radio-label input[type="radio"]:checked + .radio-content {
        color: var(--primary-color);
    }

    .radio-content {
        flex: 1;
        display: flex;
        flex-direction: column;
        gap: 4px;
    }

    .radio-content strong {
        font-size: 1.125rem;
    }

    .radio-content small {
        color: var(--text-secondary);
        font-size: 0.875rem;
    }

    .checkbox-label {
        display: flex;
        align-items: center;
        gap: 8px;
        cursor: pointer;
    }

    .checkbox-label input[type="checkbox"] {
        cursor: pointer;
        width: 18px;
        height: 18px;
    }
`;
document.head.appendChild(style);
