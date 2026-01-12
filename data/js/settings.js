// === Конфигурация ===
const API_BASE = '';

// === Инициализация ===
document.addEventListener('DOMContentLoaded', () => {
    console.log('Settings page initialized');
    loadCurrentSettings();
    
    // Обновляем системную информацию каждые 5 секунд
    setInterval(loadSystemInfo, 5000);
    
    // Обновляем активную тему в карточках при загрузке
    setTimeout(() => {
        updateThemeCards();
    }, 200);
});

// Обновление активной темы в карточках
function updateThemeCards() {
    const savedTheme = localStorage.getItem('theme') || 'light';
    console.log('Обновление карточек, активная тема:', savedTheme);
    
    const cards = document.querySelectorAll('.theme-option-card');
    console.log('Найдено карточек:', cards.length);
    
    cards.forEach(card => {
        const themeKey = card.getAttribute('data-theme-key');
        if (themeKey === savedTheme) {
            card.classList.add('active');
            console.log('Активирована карточка:', themeKey);
        } else {
            card.classList.remove('active');
        }
    });
}

// Функция для применения темы при клике на карточку
function applyThemeClick(theme, event) {
    console.log('Клик по теме:', theme);
    
    // Предотвращаем стандартное поведение если event передан
    if (event) {
        event.preventDefault();
        event.stopPropagation();
    }
    
    // Применяем тему напрямую к элементу html
    const htmlElement = document.documentElement;
    
    if (theme === 'light') {
        htmlElement.removeAttribute('data-theme');
        console.log('Удален атрибут data-theme');
    } else {
        htmlElement.setAttribute('data-theme', theme);
        console.log('Установлен атрибут data-theme:', theme);
    }
    
    // Проверяем, что атрибут установлен
    const currentTheme = htmlElement.getAttribute('data-theme') || 'light';
    console.log('Текущая тема после установки:', currentTheme);
    
    // Сохраняем в localStorage
    localStorage.setItem('theme', theme);
    console.log('Тема сохранена в localStorage:', theme);
    
    // Обновляем карточки
    updateThemeCards();
    
    // Если ThemeManager доступен, используем его для дополнительных обновлений
    if (window.ThemeManager) {
        window.ThemeManager.currentTheme = theme;
        window.ThemeManager.updateChartsTheme();
    }
    
    console.log('Тема применена:', theme);
    showNotification('Тема применена: ' + (window.ThemeManager?.themes[theme]?.name || theme), 'success');
    return false;
}

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

    // Загрузка параметров измерения
    loadMeasurementSettings();
    
    // Загрузка настроек температуры
    loadTemperatureSettings();

    // Загрузка системной информации
    loadSystemInfo();
}

// === Загрузка параметров измерения ===
async function loadMeasurementSettings() {
    try {
        const response = await fetch(`${API_BASE}/api/settings/measurement`);
        if (response.ok) {
            const data = await response.json();
            document.getElementById('samples').value = data.samples || 100;
            document.getElementById('delay').value = data.delay || 10;
            document.getElementById('interval').value = data.interval || 5;
        }
    } catch (error) {
        console.error('Error loading measurement settings:', error);
    }
}

// === Загрузка настроек температурной компенсации ===
async function loadTemperatureSettings() {
    try {
        const response = await fetch(`${API_BASE}/api/settings/temperature`);
        if (response.ok) {
            const data = await response.json();
            document.getElementById('tempReference').value = data.temperature_reference || 20.0;
            document.getElementById('tempCoefficient').value = data.temperature_coefficient || 0.4;
        }
    } catch (error) {
        console.error('Error loading temperature settings:', error);
    }
}

// === Загрузка системной информации ===
async function loadSystemInfo() {
    try {
        const response = await fetch(`${API_BASE}/api/system/info`);
        if (!response.ok) throw new Error('Failed to fetch system info');

        const data = await response.json();

        // Свободная память
        if (data.free_heap !== undefined) {
            document.getElementById('freeHeap').textContent = `${data.free_heap} KB`;
        } else {
            document.getElementById('freeHeap').textContent = '-- KB';
        }

        // Uptime (в секундах)
        if (data.uptime !== undefined) {
            document.getElementById('uptime').textContent = formatUptime(data.uptime * 1000);
        } else {
            document.getElementById('uptime').textContent = '--';
        }

        // Файловая система
        if (data.fs_total !== undefined && data.fs_used !== undefined) {
            document.getElementById('fsInfo').textContent = `${data.fs_used} / ${data.fs_total} KB`;
        } else {
            document.getElementById('fsInfo').textContent = '-- / -- KB';
        }
    } catch (error) {
        console.error('Error loading system info:', error);
        document.getElementById('freeHeap').textContent = '-- KB';
        document.getElementById('uptime').textContent = '--';
        document.getElementById('fsInfo').textContent = '-- / -- KB';
    }
}

// === Сканирование WiFi сетей ===
async function scanWiFi() {
    const scanBtn = document.getElementById('scanBtn');
    const wifiListGroup = document.getElementById('wifiListGroup');
    const wifiList = document.getElementById('wifiList');
    
    // Блокируем кнопку и показываем загрузку
    scanBtn.disabled = true;
    scanBtn.innerHTML = '<span class="btn-icon">⏳</span><span class="btn-text">Сканирование...</span>';
    wifiList.innerHTML = '';
    wifiListGroup.style.display = 'none';
    
    try {
        const response = await fetch(`${API_BASE}/api/wifi/scan`);
        if (!response.ok) throw new Error('Failed to scan WiFi');
        
        const data = await response.json();
        const networks = data.networks || [];
        
        if (networks.length === 0) {
            wifiList.innerHTML = '<div class="wifi-item empty">Сети не найдены</div>';
            wifiListGroup.style.display = 'block';
        } else {
            // Сортируем по силе сигнала (RSSI)
            networks.sort((a, b) => b.rssi - a.rssi);
            
            networks.forEach(network => {
                const item = document.createElement('div');
                item.className = 'wifi-item';
                item.onclick = () => selectWiFiNetwork(network.ssid, network.encryption !== 'open');
                
                const signalStrength = getSignalStrength(network.rssi);
                const lockIcon = network.encryption !== 'open' ? '🔒' : '';
                
                item.innerHTML = `
                    <div class="wifi-item-content">
                        <div class="wifi-item-main">
                            <span class="wifi-ssid">${escapeHtml(network.ssid)}</span>
                            <span class="wifi-lock">${lockIcon}</span>
                        </div>
                        <div class="wifi-item-info">
                            <span class="wifi-signal">${signalStrength}</span>
                            <span class="wifi-channel">Канал ${network.channel}</span>
                        </div>
                    </div>
                `;
                
                wifiList.appendChild(item);
            });
            
            wifiListGroup.style.display = 'block';
        }
        
        showNotification(`Найдено сетей: ${networks.length}`, 'success');
    } catch (error) {
        console.error('Error scanning WiFi:', error);
        showNotification('Ошибка сканирования WiFi сетей', 'error');
        wifiList.innerHTML = '<div class="wifi-item error">Ошибка сканирования</div>';
        wifiListGroup.style.display = 'block';
    } finally {
        scanBtn.disabled = false;
        scanBtn.innerHTML = '<span class="btn-icon">🔍</span><span class="btn-text">Сканировать</span>';
    }
}

// === Выбор WiFi сети ===
function selectWiFiNetwork(ssid, isEncrypted) {
    document.getElementById('ssid').value = ssid;
    if (isEncrypted) {
        document.getElementById('password').focus();
    } else {
        document.getElementById('password').value = '';
    }
    
    // Подсвечиваем выбранную сеть
    document.querySelectorAll('.wifi-item').forEach(item => {
        item.classList.remove('selected');
        if (item.querySelector('.wifi-ssid').textContent === ssid) {
            item.classList.add('selected');
        }
    });
}

// === Получить силу сигнала ===
function getSignalStrength(rssi) {
    if (rssi >= -50) return '📶📶📶';
    if (rssi >= -60) return '📶📶';
    if (rssi >= -70) return '📶';
    return '📡';
}

// === Переключение видимости пароля ===
function togglePasswordVisibility() {
    const passwordInput = document.getElementById('password');
    const passwordToggle = document.getElementById('passwordToggle');
    const eyeIcon = passwordToggle.querySelector('.eye-icon');
    
    if (passwordInput.type === 'password') {
        passwordInput.type = 'text';
        eyeIcon.textContent = '🙈';
        eyeIcon.title = 'Скрыть пароль';
    } else {
        passwordInput.type = 'password';
        eyeIcon.textContent = '👁️';
        eyeIcon.title = 'Показать пароль';
    }
}

// === Экранирование HTML ===
function escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

// === Сохранение Wi-Fi настроек ===
async function saveWifiSettings(event) {
    if (event) {
        event.preventDefault();
    }

    const ssid = document.getElementById('ssid').value;
    const password = document.getElementById('password').value;

    if (!ssid) {
        showNotification('Введите SSID сети', 'error');
        return;
    }

    if (!confirm('Устройство будет перезагружено для применения настроек. Продолжить?')) {
        return;
    }

    console.log('Сохранение Wi-Fi настроек:', { ssid, passwordLength: password.length });

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

        console.log('Ответ сервера:', response.status, response.statusText);

        if (response.ok) {
            const data = await response.json().catch(() => ({}));
            console.log('Данные ответа:', data);
            showNotification('Настройки сохранены. Устройство перезагружается...', 'success');

            // Через 5 секунд пробуем переподключиться
            setTimeout(() => {
                window.location.reload();
            }, 5000);
        } else {
            const errorText = await response.text().catch(() => 'Unknown error');
            console.error('Ошибка ответа сервера:', response.status, errorText);
            throw new Error(`Failed to save: ${response.status} ${errorText}`);
        }
    } catch (error) {
        console.error('Error saving Wi-Fi settings:', error);
        showNotification('Ошибка сохранения настроек Wi-Fi: ' + error.message, 'error');
    }
}

// === Сохранение параметров измерения ===
async function saveMeasurementSettings(event) {
    if (event) {
        event.preventDefault();
    }

    const samples = parseInt(document.getElementById('samples').value);
    const delay = parseInt(document.getElementById('delay').value);
    const interval = parseInt(document.getElementById('interval').value);

    console.log('Сохранение параметров измерения:', { samples, delay, interval });

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

        console.log('Ответ сервера:', response.status, response.statusText);

        if (response.ok) {
            const data = await response.json().catch(() => ({}));
            console.log('Данные ответа:', data);
            showNotification('Параметры измерения сохранены!', 'success');
        } else {
            const errorText = await response.text().catch(() => 'Unknown error');
            console.error('Ошибка ответа сервера:', response.status, errorText);
            throw new Error(`Failed to save: ${response.status} ${errorText}`);
        }
    } catch (error) {
        console.error('Error saving measurement settings:', error);
        showNotification('Ошибка сохранения параметров: ' + error.message, 'error');
    }
}

// === Сохранение настроек температурной компенсации ===
async function saveTempSettings(event) {
    if (event) {
        event.preventDefault();
    }

    const tempReference = parseFloat(document.getElementById('tempReference').value);
    const tempCoefficient = parseFloat(document.getElementById('tempCoefficient').value);

    console.log('Сохранение настроек температуры:', { tempReference, tempCoefficient });

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

        console.log('Ответ сервера:', response.status, response.statusText);

        if (response.ok) {
            const data = await response.json().catch(() => ({}));
            console.log('Данные ответа:', data);
            showNotification('Настройки температурной компенсации сохранены!', 'success');
        } else {
            const errorText = await response.text().catch(() => 'Unknown error');
            console.error('Ошибка ответа сервера:', response.status, errorText);
            throw new Error(`Failed to save: ${response.status} ${errorText}`);
        }
    } catch (error) {
        console.error('Error saving temperature settings:', error);
        showNotification('Ошибка сохранения настроек: ' + error.message, 'error');
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
    // ms может быть числом (миллисекунды) или уже в секундах
    let totalSeconds = Math.floor(ms / 1000);
    
    const days = Math.floor(totalSeconds / 86400);
    const hours = Math.floor((totalSeconds % 86400) / 3600);
    const minutes = Math.floor((totalSeconds % 3600) / 60);
    const seconds = totalSeconds % 60;

    if (days > 0) {
        return `${days}д ${hours}ч`;
    } else if (hours > 0) {
        return `${hours}ч ${minutes}м`;
    } else if (minutes > 0) {
        return `${minutes}м ${seconds}с`;
    } else {
        return `${seconds}с`;
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

    /* WiFi сканирование */
    .input-with-button {
        display: flex;
        gap: 8px;
        align-items: stretch;
    }

    .input-with-button input {
        flex: 1;
    }

    .btn-scan {
        white-space: nowrap;
        display: flex;
        align-items: center;
        gap: 6px;
        padding: 12px 16px;
    }

    .btn-icon {
        font-size: 1rem;
    }

    .btn-text {
        font-size: 0.875rem;
    }

    .wifi-list {
        max-height: 300px;
        overflow-y: auto;
        border: 1px solid var(--border-color);
        border-radius: var(--radius);
        background: var(--bg-color);
        margin-top: 8px;
    }

    .wifi-item {
        padding: 12px;
        border-bottom: 1px solid var(--border-color);
        cursor: pointer;
        transition: background-color 0.2s;
    }

    .wifi-item:last-child {
        border-bottom: none;
    }

    .wifi-item:hover {
        background-color: var(--primary-light);
    }

    .wifi-item.selected {
        background-color: var(--primary-light);
        border-left: 3px solid var(--primary-color);
    }

    .wifi-item.empty,
    .wifi-item.error {
        text-align: center;
        color: var(--text-secondary);
        cursor: default;
    }

    .wifi-item-content {
        display: flex;
        justify-content: space-between;
        align-items: center;
    }

    .wifi-item-main {
        display: flex;
        align-items: center;
        gap: 8px;
        flex: 1;
    }

    .wifi-ssid {
        font-weight: 500;
        color: var(--text-primary);
    }

    .wifi-lock {
        font-size: 0.875rem;
    }

    .wifi-item-info {
        display: flex;
        align-items: center;
        gap: 12px;
        font-size: 0.875rem;
        color: var(--text-secondary);
    }

    .wifi-signal {
        font-size: 1rem;
    }

    .wifi-channel {
        white-space: nowrap;
    }

    /* Пароль с иконкой глаза */
    .password-input-wrapper {
        position: relative;
        display: flex;
        align-items: center;
    }

    .password-input-wrapper input {
        padding-right: 45px;
    }

    .password-toggle {
        position: absolute;
        right: 8px;
        background: none;
        border: none;
        cursor: pointer;
        padding: 8px;
        display: flex;
        align-items: center;
        justify-content: center;
        border-radius: var(--radius);
        transition: background-color 0.2s;
    }

    .password-toggle:hover {
        background-color: var(--bg-color);
    }

    .password-toggle:active {
        background-color: var(--border-color);
    }

    .eye-icon {
        font-size: 1.25rem;
        user-select: none;
    }

    @media (max-width: 768px) {
        .header-actions {
            flex-direction: column;
        }

        .input-with-button {
            flex-direction: column;
        }

        .btn-scan {
            width: 100%;
            justify-content: center;
        }

        .wifi-item-content {
            flex-direction: column;
            align-items: flex-start;
            gap: 8px;
        }

        .wifi-item-info {
            width: 100%;
            justify-content: space-between;
        }
    }
`;
document.head.appendChild(style);
