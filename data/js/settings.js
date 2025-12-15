// === Конфигурация ===
const API_BASE = '';

// === Инициализация ===
document.addEventListener('DOMContentLoaded', () => {
    console.log('Settings page initialized');
    loadCurrentSettings();
});

// === Загрузка текущих настроек ===
async function loadCurrentSettings() {
    try {
        const response = await fetch(`${API_BASE}/api/status`);
        if (!response.ok) throw new Error('Failed to fetch status');

        const data = await response.json();

        // Обновляем отображение
        document.getElementById('currentSSID').textContent = data.ssid || '--';
        document.getElementById('currentIP').textContent = data.ip || '--';
        document.getElementById('wifiMode').textContent = data.wifi_mode || '--';
        document.getElementById('firmwareVersion').textContent = data.firmware || '--';

        // Заполняем форму
        document.getElementById('ssid').value = data.ssid || '';
    } catch (error) {
        console.error('Error loading settings:', error);
    }

    // Загрузка системной информации
    loadSystemInfo();
}

// === Загрузка системной информации ===
async function loadSystemInfo() {
    try {
        // В реальности нужен отдельный endpoint для системной информации
        // Пока используем заглушки
        document.getElementById('freeHeap').textContent = '-- KB';
        document.getElementById('uptime').textContent = formatUptime(Date.now());
        document.getElementById('fsInfo').textContent = '-- / -- KB';
    } catch (error) {
        console.error('Error loading system info:', error);
    }
}

// === Сохранение Wi-Fi настроек ===
async function saveWifiSettings(event) {
    event.preventDefault();

    const ssid = document.getElementById('ssid').value;
    const password = document.getElementById('password').value;

    if (!ssid) {
        showNotification('Введите SSID сети', 'error');
        return;
    }

    if (!confirm('Устройство будет перезагружено для применения настроек. Продолжить?')) {
        return;
    }

    try {
        const response = await fetch(`${API_BASE}/api/wifi/config`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                ssid: ssid,
                password: password
            })
        });

        if (response.ok) {
            showNotification('Настройки сохранены. Устройство перезагружается...', 'success');

            // Через 5 секунд пробуем переподключиться
            setTimeout(() => {
                window.location.reload();
            }, 5000);
        } else {
            throw new Error('Failed to save Wi-Fi settings');
        }
    } catch (error) {
        console.error('Error saving Wi-Fi settings:', error);
        showNotification('Ошибка сохранения настроек Wi-Fi', 'error');
    }
}

// === Сохранение параметров измерения ===
async function saveMeasurementSettings(event) {
    event.preventDefault();

    const samples = parseInt(document.getElementById('samples').value);
    const delay = parseInt(document.getElementById('delay').value);
    const interval = parseInt(document.getElementById('interval').value);

    try {
        const response = await fetch(`${API_BASE}/api/settings/measurement`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                samples: samples,
                delay: delay,
                interval: interval
            })
        });

        if (response.ok) {
            showNotification('Параметры измерения сохранены!', 'success');
        } else {
            throw new Error('Failed to save measurement settings');
        }
    } catch (error) {
        console.error('Error saving measurement settings:', error);
        showNotification('Ошибка сохранения параметров', 'error');
    }
}

// === Сохранение настроек температурной компенсации ===
async function saveTempSettings(event) {
    event.preventDefault();

    const tempReference = parseFloat(document.getElementById('tempReference').value);
    const tempCoefficient = parseFloat(document.getElementById('tempCoefficient').value);

    try {
        const response = await fetch(`${API_BASE}/api/settings/temperature`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                temperature_reference: tempReference,
                temperature_coefficient: tempCoefficient
            })
        });

        if (response.ok) {
            showNotification('Настройки температурной компенсации сохранены!', 'success');
        } else {
            throw new Error('Failed to save temperature settings');
        }
    } catch (error) {
        console.error('Error saving temperature settings:', error);
        showNotification('Ошибка сохранения настроек', 'error');
    }
}

// === Действия ===
async function rebootDevice() {
    if (!confirm('Вы уверены, что хотите перезагрузить устройство?')) {
        return;
    }

    try {
        const response = await fetch(`${API_BASE}/api/reboot`, {
            method: 'POST'
        });

        if (response.ok) {
            showNotification('Устройство перезагружается...', 'success');
            setTimeout(() => {
                window.location.reload();
            }, 5000);
        } else {
            throw new Error('Failed to reboot');
        }
    } catch (error) {
        console.error('Error rebooting:', error);
        showNotification('Ошибка перезагрузки', 'error');
    }
}

async function resetSettings() {
    if (!confirm('Это сбросит все настройки к значениям по умолчанию. Продолжить?')) {
        return;
    }

    try {
        const response = await fetch(`${API_BASE}/api/reset`, {
            method: 'POST'
        });

        if (response.ok) {
            showNotification('Настройки сброшены. Устройство перезагружается...', 'success');
            setTimeout(() => {
                window.location.reload();
            }, 3000);
        } else {
            throw new Error('Failed to reset settings');
        }
    } catch (error) {
        console.error('Error resetting:', error);
        showNotification('Ошибка сброса настроек', 'error');
    }
}

async function clearLogs() {
    if (!confirm('Это удалит всю историю измерений. Продолжить?')) {
        return;
    }

    try {
        const response = await fetch(`${API_BASE}/api/logs`, {
            method: 'DELETE'
        });

        if (response.ok) {
            showNotification('Логи очищены!', 'success');
        } else {
            throw new Error('Failed to clear logs');
        }
    } catch (error) {
        console.error('Error clearing logs:', error);
        showNotification('Ошибка очистки логов', 'error');
    }
}

async function exportSettings() {
    try {
        // Собираем все настройки
        const settings = {
            wifi: {
                ssid: document.getElementById('ssid').value
            },
            measurement: {
                samples: parseInt(document.getElementById('samples').value),
                delay: parseInt(document.getElementById('delay').value),
                interval: parseInt(document.getElementById('interval').value)
            },
            temperature: {
                reference: parseFloat(document.getElementById('tempReference').value),
                coefficient: parseFloat(document.getElementById('tempCoefficient').value)
            },
            exported_at: new Date().toISOString()
        };

        const blob = new Blob([JSON.stringify(settings, null, 2)], {
            type: 'application/json'
        });

        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = `areometr-settings-${Date.now()}.json`;
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);

        showNotification('Настройки экспортированы!', 'success');
    } catch (error) {
        console.error('Error exporting settings:', error);
        showNotification('Ошибка экспорта настроек', 'error');
    }
}

// === Утилиты ===
function formatUptime(ms) {
    const seconds = Math.floor(ms / 1000);
    const minutes = Math.floor(seconds / 60);
    const hours = Math.floor(minutes / 60);
    const days = Math.floor(hours / 24);

    if (days > 0) {
        return `${days}д ${hours % 24}ч`;
    } else if (hours > 0) {
        return `${hours}ч ${minutes % 60}м`;
    } else {
        return `${minutes}м ${seconds % 60}с`;
    }
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

// === Стили для форм ===
const style = document.createElement('style');
style.textContent = `
    .form-group {
        margin-bottom: var(--spacing);
    }

    .form-group label {
        display: block;
        font-weight: 500;
        margin-bottom: 8px;
        color: var(--text-primary);
    }

    .form-group input,
    .form-group select {
        width: 100%;
        padding: 12px;
        border: 1px solid var(--border-color);
        border-radius: var(--radius);
        font-size: 1rem;
        transition: border-color 0.3s;
    }

    .form-group input:focus,
    .form-group select:focus {
        outline: none;
        border-color: var(--primary-color);
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

    .current-wifi {
        margin-top: calc(var(--spacing) * 2);
        padding-top: calc(var(--spacing) * 2);
        border-top: 1px solid var(--border-color);
    }

    .current-wifi h3 {
        margin-bottom: var(--spacing);
        font-size: 1.125rem;
    }

    .warning-box {
        margin-top: var(--spacing);
        padding: var(--spacing);
        background: #FFF3E0;
        border-left: 4px solid var(--warning-color);
        border-radius: var(--radius);
    }

    .warning-box strong {
        display: block;
        margin-bottom: 8px;
        color: var(--warning-color);
    }

    .warning-box ul {
        margin: 0;
        padding-left: 20px;
    }

    .warning-box li {
        color: #E65100;
        margin-bottom: 4px;
    }

    .header-actions {
        display: flex;
        gap: 8px;
    }

    @media (max-width: 768px) {
        .header-actions {
            flex-direction: column;
        }
    }
`;
document.head.appendChild(style);
