// === Конфигурация ===
const API_BASE = '';
const UPDATE_INTERVAL = 5000; // 5 секунд

// === Глобальные переменные ===
let updateTimer = null;
let publishedCount = 0;

// === Инициализация ===
document.addEventListener('DOMContentLoaded', () => {
    console.log('MQTT settings page initialized');
    loadMQTTSettings();
    updateMQTTStatus();
    startStatusMonitoring();

    // Обновление топиков при изменении базового топика
    document.getElementById('mqttBaseTopicInput').addEventListener('input', updateTopicsDisplay);
});

// === Загрузка настроек ===
async function loadMQTTSettings() {
    try {
        const response = await fetch(`${API_BASE}/api/mqtt/config`);
        if (!response.ok) {
            console.log('MQTT config not available, using defaults');
            return;
        }

        const config = await response.json();

        document.getElementById('mqttEnabled').checked = config.enabled || false;
        document.getElementById('mqttServer').value = config.server || '';
        document.getElementById('mqttPort').value = config.port || 1883;
        document.getElementById('mqttUser').value = config.username || '';
        document.getElementById('mqttPassword').value = ''; // Не показываем пароль
        document.getElementById('mqttClientId').value = config.client_id || 'smart-areometr';
        document.getElementById('mqttBaseTopicInput').value = config.base_topic || 'distillery/areometer';
        document.getElementById('mqttPublishInterval').value = config.publish_interval || 5;
        document.getElementById('haDiscovery').checked = config.ha_discovery !== false;

        updateTopicsDisplay();

    } catch (error) {
        console.error('Error loading MQTT settings:', error);
    }
}

// === Сохранение настроек ===
async function saveMQTTSettings(event) {
    event.preventDefault();

    const config = {
        enabled: document.getElementById('mqttEnabled').checked,
        server: document.getElementById('mqttServer').value,
        port: parseInt(document.getElementById('mqttPort').value),
        username: document.getElementById('mqttUser').value,
        password: document.getElementById('mqttPassword').value,
        client_id: document.getElementById('mqttClientId').value,
        base_topic: document.getElementById('mqttBaseTopicInput').value,
        publish_interval: parseInt(document.getElementById('mqttPublishInterval').value),
        ha_discovery: document.getElementById('haDiscovery').checked
    };

    try {
        const response = await fetch(`${API_BASE}/api/mqtt/config`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify(config)
        });

        if (!response.ok) throw new Error('Failed to save settings');

        showNotification('Настройки MQTT сохранены! Перезагрузите устройство.', 'success');

        // Обновляем статус через 2 секунды
        setTimeout(updateMQTTStatus, 2000);

    } catch (error) {
        console.error('Error saving MQTT settings:', error);
        showNotification('Ошибка сохранения настроек MQTT', 'error');
    }
}

// === Обновление статуса MQTT ===
async function updateMQTTStatus() {
    try {
        const response = await fetch(`${API_BASE}/api/mqtt/status`);
        if (!response.ok) {
            updateStatusDisplay(false, 'Не доступен', '');
            return;
        }

        const status = await response.json();

        const connected = status.connected || false;
        const broker = status.server ? `${status.server}:${status.port}` : '-';
        const baseTopic = status.base_topic || '-';

        updateStatusDisplay(connected, broker, baseTopic);

        if (status.published_count !== undefined) {
            publishedCount = status.published_count;
            document.getElementById('mqttPublishedCount').textContent = publishedCount;
        }

    } catch (error) {
        console.error('Error fetching MQTT status:', error);
        updateStatusDisplay(false, 'Ошибка', '');
    }
}

// === Обновление отображения статуса ===
function updateStatusDisplay(connected, broker, baseTopic) {
    const statusElement = document.getElementById('mqttConnectionStatus');

    if (connected) {
        statusElement.innerHTML = '<span style="color: #4CAF50;">● Подключено</span>';
    } else {
        statusElement.innerHTML = '<span style="color: #F44336;">○ Отключено</span>';
    }

    document.getElementById('mqttBrokerInfo').textContent = broker;
    document.getElementById('mqttBaseTopic').textContent = baseTopic;
}

// === Обновление отображения топиков ===
function updateTopicsDisplay() {
    const baseTopic = document.getElementById('mqttBaseTopicInput').value;

    document.getElementById('topicState').textContent = `${baseTopic}/state`;
    document.getElementById('topicAlcohol').textContent = `${baseTopic}/alcohol`;
    document.getElementById('topicTemp').textContent = `${baseTopic}/temperature`;
    document.getElementById('topicStability').textContent = `${baseTopic}/stability`;
    document.getElementById('topicFraction').textContent = `${baseTopic}/fraction`;
    document.getElementById('topicAvail').textContent = `${baseTopic}/availability`;
    document.getElementById('topicCommand').textContent = `${baseTopic}/command`;
    document.getElementById('topicCalibrate').textContent = `${baseTopic}/calibrate`;
    document.getElementById('topicThresholds').textContent = `${baseTopic}/set_thresholds`;
}

// === Тест подключения ===
async function testConnection() {
    showNotification('Тестирование подключения...', 'info');

    try {
        const response = await fetch(`${API_BASE}/api/mqtt/test`, {
            method: 'POST'
        });

        if (!response.ok) throw new Error('Test failed');

        const result = await response.json();

        if (result.success) {
            showNotification('Подключение успешно! Broker доступен.', 'success');
        } else {
            showNotification(`Ошибка подключения: ${result.error || 'Unknown'}`, 'error');
        }

    } catch (error) {
        console.error('Error testing connection:', error);
        showNotification('Ошибка теста подключения', 'error');
    }
}

// === Переподключение ===
async function reconnect() {
    showNotification('Переподключение к MQTT broker...', 'info');

    // Переподключение происходит автоматически через handle()
    // Просто обновляем статус
    setTimeout(updateMQTTStatus, 3000);
    showNotification('Проверка подключения...', 'info');
}

// === Мониторинг статуса ===
function startStatusMonitoring() {
    if (updateTimer) clearInterval(updateTimer);

    updateTimer = setInterval(updateMQTTStatus, UPDATE_INTERVAL);
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
    }, 4000);
}

// === Очистка при уходе со страницы ===
document.addEventListener('visibilitychange', () => {
    if (document.hidden) {
        if (updateTimer) clearInterval(updateTimer);
    } else {
        startStatusMonitoring();
        updateMQTTStatus();
    }
});

// Добавляем анимации
const style = document.createElement('style');
style.textContent = `
    .mqtt-status {
        padding: var(--spacing);
    }

    .form-group {
        margin-bottom: var(--spacing);
    }

    .form-group label {
        display: block;
        font-weight: 500;
        margin-bottom: 8px;
        color: var(--text-primary);
    }

    .form-group input[type="text"],
    .form-group input[type="number"],
    .form-group input[type="password"] {
        width: 100%;
        padding: 12px;
        border: 1px solid var(--border-color);
        border-radius: var(--radius);
        font-size: 1rem;
        transition: border-color 0.3s;
        background: var(--card-bg);
        color: var(--text-primary);
    }

    .form-group input:focus {
        outline: none;
        border-color: var(--primary-color);
    }

    .form-group input[type="checkbox"] {
        margin-right: 8px;
    }

    .form-group small {
        display: block;
        margin-top: 4px;
        color: var(--text-secondary);
        font-size: 0.875rem;
    }

    .form-actions {
        margin-top: calc(var(--spacing) * 1.5);
    }

    .status-row {
        display: flex;
        justify-content: space-between;
        padding: 12px 0;
        border-bottom: 1px solid var(--border-color);
    }

    .status-row:last-child {
        border-bottom: none;
    }

    .status-label {
        font-weight: 500;
        color: var(--text-secondary);
    }

    .status-value {
        font-weight: 600;
        color: var(--text-primary);
    }

    .button-group {
        display: flex;
        gap: var(--spacing);
        margin-top: calc(var(--spacing) * 2);
    }

    .collapsible-section {
        margin-top: var(--spacing);
    }

    .collapsible-summary {
        cursor: pointer;
        padding: 12px;
        background: var(--bg-color);
        border-radius: var(--radius);
        font-weight: 500;
        color: var(--primary-color);
        user-select: none;
        transition: background-color 0.2s;
        list-style: none;
        display: flex;
        align-items: center;
        gap: 8px;
    }

    .collapsible-summary::before {
        content: '▶';
        display: inline-block;
        transition: transform 0.2s;
        font-size: 0.875rem;
    }

    .collapsible-section[open] .collapsible-summary::before {
        transform: rotate(90deg);
    }

    .collapsible-summary:hover {
        background: var(--primary-light);
    }

    .collapsible-summary::-webkit-details-marker {
        display: none;
    }

    .collapsible-section[open] .collapsible-summary {
        margin-bottom: var(--spacing);
    }

    .topics-info h3 {
        margin-top: calc(var(--spacing) * 1.5);
        margin-bottom: var(--spacing);
        color: var(--primary-color);
    }

    .topics-list {
        list-style: none;
        padding: 0;
        margin: var(--spacing) 0;
    }

    .topics-list li {
        padding: 8px 0;
        font-family: monospace;
    }

    .topics-list code {
        background: var(--bg-color);
        padding: 4px 8px;
        border-radius: 4px;
        color: var(--primary-color);
        font-weight: 600;
    }

    .topic-desc {
        color: var(--text-secondary);
        font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial, sans-serif;
        font-size: 0.875rem;
        margin-left: 8px;
    }

    .examples {
        line-height: 1.8;
    }

    .examples h3 {
        margin-top: calc(var(--spacing) * 1.5);
        margin-bottom: var(--spacing);
        color: var(--primary-color);
    }

    .examples pre {
        background: var(--bg-color);
        padding: var(--spacing);
        border-radius: var(--radius);
        border-left: 4px solid var(--primary-color);
        overflow-x: auto;
        font-size: 0.875rem;
    }

    .examples code {
        color: var(--text-primary);
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
