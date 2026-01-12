// Система интернационализации (i18n) для Smart Areometr
// Поддержка русского и английского языков

const i18n = {
    currentLanguage: 'ru',  // Текущий язык по умолчанию
    translations: {
        ru: {
            // Общие
            'app.title': 'Smart Areometr',
            'app.loading': 'Загрузка...',
            'app.error': 'Ошибка',
            'app.success': 'Успешно',
            'app.save': 'Сохранить',
            'app.cancel': 'Отмена',
            'app.delete': 'Удалить',
            'app.edit': 'Редактировать',
            'app.close': 'Закрыть',
            
            // Навигация
            'nav.home': 'Главная',
            'nav.calibration': 'Калибровка',
            'nav.fractions': 'Фракции',
            'nav.receivers': 'Приемники',
            'nav.sessions': 'Сессии',
            'nav.mqtt': 'MQTT',
            'nav.settings': 'Настройки',
            'nav.logs': 'Логи',
            
            // Главная страница
            'home.title': 'Главная',
            'home.alcohol': 'Крепость',
            'home.temperature': 'Температура',
            'home.stability': 'Стабильность',
            'home.fraction': 'Фракция',
            'home.measure': 'Измерить',
            'home.connected': 'Подключено',
            'home.disconnected': 'Отключено',
            
            // Калибровка
            'calibration.title': 'Калибровка',
            'calibration.add_point': 'Добавить точку',
            'calibration.clear': 'Очистить',
            'calibration.points': 'Точки калибровки',
            'calibration.water': 'Вода (0%)',
            'calibration.alcohol': 'Спирт (100%)',
            
            // Фракции
            'fractions.title': 'Фракции',
            'fractions.current': 'Текущая фракция',
            'fractions.volume': 'Объем',
            'fractions.stats': 'Статистика',
            
            // Приемники
            'receivers.title': 'Приемники',
            'receivers.status': 'Статус приемников',
            'receivers.active': 'АКТИВЕН',
            'receivers.overflow': 'ПЕРЕПОЛНЕНИЕ',
            'receivers.volume': 'Объем',
            'receivers.switch': 'Переключить',
            'receivers.auto_switch': 'Авто-переключение',
            
            // Сессии
            'sessions.title': 'Сессии дистилляции',
            'sessions.start': 'Начать',
            'sessions.stop': 'Остановить',
            'sessions.pause': 'Пауза',
            'sessions.export': 'Экспорт',
            
            // MQTT
            'mqtt.title': 'MQTT настройки',
            'mqtt.enabled': 'Включено',
            'mqtt.server': 'Сервер',
            'mqtt.port': 'Порт',
            'mqtt.test': 'Тест подключения',
            
            // Настройки
            'settings.title': 'Настройки',
            'settings.language': 'Язык',
            'settings.theme': 'Тема',
            'settings.wifi': 'Wi-Fi',
            
            // Логи
            'logs.title': 'Логи измерений',
            'logs.export': 'Экспорт CSV',
            'logs.stats': 'Статистика',
            
            // Батарея
            'battery.voltage': 'Напряжение',
            'battery.percent': 'Заряд',
            'battery.charging': 'Зарядка',
            'battery.low': 'Низкий заряд',
            'battery.critical': 'Критический заряд'
        },
        en: {
            // General
            'app.title': 'Smart Areometr',
            'app.loading': 'Loading...',
            'app.error': 'Error',
            'app.success': 'Success',
            'app.save': 'Save',
            'app.cancel': 'Cancel',
            'app.delete': 'Delete',
            'app.edit': 'Edit',
            'app.close': 'Close',
            
            // Navigation
            'nav.home': 'Home',
            'nav.calibration': 'Calibration',
            'nav.fractions': 'Fractions',
            'nav.receivers': 'Receivers',
            'nav.sessions': 'Sessions',
            'nav.mqtt': 'MQTT',
            'nav.settings': 'Settings',
            'nav.logs': 'Logs',
            
            // Home page
            'home.title': 'Home',
            'home.alcohol': 'Alcohol',
            'home.temperature': 'Temperature',
            'home.stability': 'Stability',
            'home.fraction': 'Fraction',
            'home.measure': 'Measure',
            'home.connected': 'Connected',
            'home.disconnected': 'Disconnected',
            
            // Calibration
            'calibration.title': 'Calibration',
            'calibration.add_point': 'Add Point',
            'calibration.clear': 'Clear',
            'calibration.points': 'Calibration Points',
            'calibration.water': 'Water (0%)',
            'calibration.alcohol': 'Alcohol (100%)',
            
            // Fractions
            'fractions.title': 'Fractions',
            'fractions.current': 'Current Fraction',
            'fractions.volume': 'Volume',
            'fractions.stats': 'Statistics',
            
            // Receivers
            'receivers.title': 'Receivers',
            'receivers.status': 'Receiver Status',
            'receivers.active': 'ACTIVE',
            'receivers.overflow': 'OVERFLOW',
            'receivers.volume': 'Volume',
            'receivers.switch': 'Switch',
            'receivers.auto_switch': 'Auto-switch',
            
            // Sessions
            'sessions.title': 'Distillation Sessions',
            'sessions.start': 'Start',
            'sessions.stop': 'Stop',
            'sessions.pause': 'Pause',
            'sessions.export': 'Export',
            
            // MQTT
            'mqtt.title': 'MQTT Settings',
            'mqtt.enabled': 'Enabled',
            'mqtt.server': 'Server',
            'mqtt.port': 'Port',
            'mqtt.test': 'Test Connection',
            
            // Settings
            'settings.title': 'Settings',
            'settings.language': 'Language',
            'settings.theme': 'Theme',
            'settings.wifi': 'Wi-Fi',
            
            // Logs
            'logs.title': 'Measurement Logs',
            'logs.export': 'Export CSV',
            'logs.stats': 'Statistics',
            
            // Battery
            'battery.voltage': 'Voltage',
            'battery.percent': 'Charge',
            'battery.charging': 'Charging',
            'battery.low': 'Low Battery',
            'battery.critical': 'Critical Battery'
        }
    },
    
    /**
     * Инициализация i18n
     */
    init() {
        // Загружаем сохраненный язык из localStorage
        const savedLang = localStorage.getItem('language') || 'ru';
        this.setLanguage(savedLang);
    },
    
    /**
     * Установить язык
     */
    setLanguage(lang) {
        if (this.translations[lang]) {
            this.currentLanguage = lang;
            localStorage.setItem('language', lang);
            this.updatePage();
        }
    },
    
    /**
     * Получить перевод
     */
    t(key, defaultValue = '') {
        const translation = this.translations[this.currentLanguage];
        if (translation && translation[key]) {
            return translation[key];
        }
        // Fallback на английский, если нет перевода
        if (this.currentLanguage !== 'en') {
            const enTranslation = this.translations['en'];
            if (enTranslation && enTranslation[key]) {
                return enTranslation[key];
            }
        }
        return defaultValue || key;
    },
    
    /**
     * Обновить все элементы страницы с data-i18n атрибутом
     */
    updatePage() {
        // Обновляем элементы с data-i18n
        document.querySelectorAll('[data-i18n]').forEach(element => {
            const key = element.getAttribute('data-i18n');
            const translation = this.t(key);
            
            if (element.tagName === 'INPUT' && element.type === 'submit') {
                element.value = translation;
            } else if (element.tagName === 'INPUT' || element.tagName === 'TEXTAREA') {
                element.placeholder = translation;
            } else {
                element.textContent = translation;
            }
        });
        
        // Обновляем title страницы
        const titleElement = document.querySelector('title');
        if (titleElement) {
            titleElement.textContent = this.t('app.title');
        }
        
        // Обновляем элементы с data-i18n-title
        document.querySelectorAll('[data-i18n-title]').forEach(element => {
            const key = element.getAttribute('data-i18n-title');
            element.title = this.t(key);
        });
        
        // Обновляем элементы с data-i18n-placeholder
        document.querySelectorAll('[data-i18n-placeholder]').forEach(element => {
            const key = element.getAttribute('data-i18n-placeholder');
            element.placeholder = this.t(key);
        });
    },
    
    /**
     * Получить текущий язык
     */
    getLanguage() {
        return this.currentLanguage;
    },
    
    /**
     * Получить список доступных языков
     */
    getAvailableLanguages() {
        return Object.keys(this.translations);
    }
};

// Инициализация при загрузке
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', () => i18n.init());
} else {
    i18n.init();
}

// Экспорт для использования в других скриптах
window.i18n = i18n;

