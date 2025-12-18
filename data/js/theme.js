// === Управление темами ===
const ThemeManager = {
    currentTheme: 'light', // По умолчанию светлая тема
    themes: {
        light: { name: 'Светлая', icon: '☀️' },
        dark: { name: 'Темная', icon: '🌙' },
        blue: { name: 'Голубая', icon: '💙' },
        purple: { name: 'Пурпурная', icon: '💜' },
        cyberpunk: { name: 'Киберпанк', icon: '🤖' },
        orange: { name: 'Оранжевая', icon: '🧡' }
    },

    // Инициализация
    init() {
        // Загружаем сохраненную тему из localStorage
        const savedTheme = localStorage.getItem('theme');
        if (savedTheme && this.themes[savedTheme]) {
            this.currentTheme = savedTheme;
        }
        
        // Применяем тему СРАЗУ (синхронно)
        this.applyThemeSync(this.currentTheme);
        
        // Создаем переключатель тем (если не на странице настроек)
        this.createThemeSwitcher();
        
        // Обновляем карточки тем в настройках (если они уже есть в HTML)
        this.updateThemeCards();
    },
    
    // Синхронное применение темы (без обновления графиков и других асинхронных операций)
    applyThemeSync(theme) {
        if (!this.themes[theme]) {
            return;
        }

        this.currentTheme = theme;
        
        // Устанавливаем атрибут data-theme на html элементе СРАЗУ
        if (theme === 'light') {
            document.documentElement.removeAttribute('data-theme');
        } else {
            document.documentElement.setAttribute('data-theme', theme);
        }
    },

    // Применение темы
    applyTheme(theme) {
        if (!this.themes[theme]) {
            console.warn(`Тема "${theme}" не найдена`);
            return;
        }

        console.log('Применение темы:', theme);
        this.currentTheme = theme;
        
        // Устанавливаем атрибут data-theme на html элементе
        this.applyThemeSync(theme);
        
        // Сохраняем в localStorage
        localStorage.setItem('theme', theme);
        
        // Обновляем переключатель
        this.updateThemeSwitcher();
        
        // Обновляем карточки тем в настройках
        this.updateThemeCards();
        
        // Обновляем графики если они есть
        this.updateChartsTheme();
        
        console.log('Тема применена:', theme);
    },
    
    // Обновление карточек тем в настройках
    updateThemeCards() {
        const cards = document.querySelectorAll('.theme-option-card');
        cards.forEach(card => {
            const themeKey = card.getAttribute('data-theme-key');
            if (themeKey === this.currentTheme) {
                card.classList.add('active');
            } else {
                card.classList.remove('active');
            }
        });
    },

    // Создание переключателя тем
    createThemeSwitcher() {
        // Проверяем, находимся ли мы на странице настроек
        const themeOptionsGrid = document.getElementById('themeOptionsGrid');
        if (themeOptionsGrid) {
            console.log('ThemeManager: найден контейнер themeOptionsGrid, создаем карточки');
            // Создаем опции тем в настройках
            this.createThemeOptionsInSettings(themeOptionsGrid);
            return;
        }

        // Если не на странице настроек, создаем переключатель в навигации (для обратной совместимости)
        if (document.getElementById('themeSwitcher')) {
            return;
        }

        const navbar = document.querySelector('.navbar');
        if (!navbar) {
            return;
        }

        // Создаем контейнер для переключателя
        const themeContainer = document.createElement('div');
        themeContainer.className = 'theme-switcher';
        themeContainer.id = 'themeSwitcher';

        // Кнопка переключателя
        const themeBtn = document.createElement('button');
        themeBtn.className = 'theme-switcher-btn';
        themeBtn.innerHTML = `
            <span class="theme-switcher-icon">${this.themes[this.currentTheme].icon}</span>
            <span class="theme-switcher-text">${this.themes[this.currentTheme].name}</span>
        `;
        themeBtn.onclick = (e) => {
            e.stopPropagation();
            this.toggleThemeMenu();
        };

        // Меню выбора темы
        const themeMenu = document.createElement('div');
        themeMenu.className = 'theme-menu';
        themeMenu.id = 'themeMenu';

        // Создаем опции для каждой темы
        Object.entries(this.themes).forEach(([key, theme]) => {
            const option = document.createElement('button');
            option.className = `theme-option ${key === this.currentTheme ? 'active' : ''}`;
            option.setAttribute('data-theme-key', key);
            option.onclick = (e) => {
                e.preventDefault();
                e.stopPropagation();
                this.applyTheme(key);
                this.toggleThemeMenu();
            };
            option.innerHTML = `
                <span class="theme-option-icon">${theme.icon}</span>
                <span class="theme-option-name">${theme.name}</span>
                <span class="theme-option-check">✓</span>
            `;
            themeMenu.appendChild(option);
        });

        themeContainer.appendChild(themeBtn);
        themeContainer.appendChild(themeMenu);
        
        // Добавляем в навигацию
        navbar.appendChild(themeContainer);

        // Закрываем меню при клике вне его
        document.addEventListener('click', (e) => {
            if (!themeContainer.contains(e.target)) {
                themeMenu.classList.remove('show');
            }
        });
    },

    // Создание опций тем в настройках
    createThemeOptionsInSettings(container) {
        if (!container) {
            console.error('ThemeManager: контейнер для карточек тем не найден');
            return;
        }
        
        // Очищаем контейнер на случай повторной инициализации
        container.innerHTML = '';
        
        console.log('ThemeManager: создание карточек тем, количество тем:', Object.keys(this.themes).length);
        
        Object.entries(this.themes).forEach(([key, theme]) => {
            const option = document.createElement('button');
            option.className = `theme-option-card ${key === this.currentTheme ? 'active' : ''}`;
            option.setAttribute('data-theme-key', key);
            option.onclick = () => {
                this.applyTheme(key);
            };
            option.innerHTML = `
                <div class="theme-option-card-icon">${theme.icon}</div>
                <div class="theme-option-card-name">${theme.name}</div>
                <div class="theme-option-card-check">✓</div>
            `;
            container.appendChild(option);
        });
        
        console.log('ThemeManager: карточки тем созданы, количество:', container.children.length);
    },

    // Переключение меню тем
    toggleThemeMenu() {
        const menu = document.getElementById('themeMenu');
        if (menu) {
            menu.classList.toggle('show');
        }
    },

    // Обновление переключателя тем
    updateThemeSwitcher() {
        // Обновляем кнопку в навигации (если есть)
        const themeBtn = document.querySelector('.theme-switcher-btn');
        const themeText = document.querySelector('.theme-switcher-text');
        const themeIcon = document.querySelector('.theme-switcher-icon');
        
        if (themeBtn && themeText && themeIcon) {
            themeText.textContent = this.themes[this.currentTheme].name;
            themeIcon.textContent = this.themes[this.currentTheme].icon;
        }

        // Обновляем активную опцию в меню навигации
        const menuOptions = document.querySelectorAll('.theme-option');
        menuOptions.forEach(option => {
            const themeKey = option.getAttribute('data-theme-key');
            if (themeKey) {
                if (themeKey === this.currentTheme) {
                    option.classList.add('active');
                } else {
                    option.classList.remove('active');
                }
            }
        });

        // Обновляем активную опцию в настройках
        const settingsOptions = document.querySelectorAll('.theme-option-card');
        settingsOptions.forEach(option => {
            const themeKey = option.getAttribute('data-theme-key');
            if (themeKey) {
                if (themeKey === this.currentTheme) {
                    option.classList.add('active');
                } else {
                    option.classList.remove('active');
                }
            }
        });
    },

    // Обновление цветов графиков Chart.js
    updateChartsTheme() {
        // Получаем цвета из CSS переменных
        const root = getComputedStyle(document.documentElement);
        const primaryColor = root.getPropertyValue('--primary-color').trim();
        const warningColor = root.getPropertyValue('--warning-color').trim() || '#FF9800';
        
        // Обновляем графики если они существуют
        if (window.alcoholChart && window.alcoholChart.data && window.alcoholChart.data.datasets) {
            const dataset = window.alcoholChart.data.datasets[0];
            if (dataset) {
                dataset.borderColor = primaryColor;
                dataset.backgroundColor = this.hexToRgba(primaryColor, 0.1);
                window.alcoholChart.update('none');
            }
        }

        if (window.temperatureChart && window.temperatureChart.data && window.temperatureChart.data.datasets) {
            const dataset = window.temperatureChart.data.datasets[0];
            if (dataset) {
                dataset.borderColor = warningColor;
                dataset.backgroundColor = this.hexToRgba(warningColor, 0.1);
                window.temperatureChart.update('none');
            }
        }
    },

    // Конвертация hex в rgba
    hexToRgba(hex, alpha) {
        const r = parseInt(hex.slice(1, 3), 16);
        const g = parseInt(hex.slice(3, 5), 16);
        const b = parseInt(hex.slice(5, 7), 16);
        return `rgba(${r}, ${g}, ${b}, ${alpha})`;
    }
};

// Инициализация при загрузке DOM
function initThemeManager() {
    ThemeManager.init();
    
    // Дополнительная проверка для страницы настроек
    // Если контейнер есть, но карточки не созданы, создаем их
    const checkAndCreate = () => {
        const themeOptionsGrid = document.getElementById('themeOptionsGrid');
        if (themeOptionsGrid) {
            if (themeOptionsGrid.children.length === 0) {
                console.log('ThemeManager: создание карточек тем (отложенная инициализация)');
                ThemeManager.createThemeOptionsInSettings(themeOptionsGrid);
            }
        } else {
            // Контейнер themeOptionsGrid есть только на странице настроек - это нормально
            // console.log('ThemeManager: контейнер themeOptionsGrid не найден');
        }
    };
    
    // Проверяем сразу и с небольшой задержкой
    checkAndCreate();
    setTimeout(checkAndCreate, 100);
    setTimeout(checkAndCreate, 500);
    setTimeout(checkAndCreate, 1000);
}

// Инициализация
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', () => {
        initThemeManager();
    });
} else {
    // DOM уже загружен
    initThemeManager();
}

// Экспорт для глобального доступа
window.ThemeManager = ThemeManager;

// Дополнительная инициализация для страницы настроек
// Вызывается после полной загрузки страницы
window.addEventListener('load', () => {
    const themeOptionsGrid = document.getElementById('themeOptionsGrid');
    if (themeOptionsGrid && themeOptionsGrid.children.length === 0 && window.ThemeManager) {
        console.log('ThemeManager: финальная проверка и создание карточек');
        window.ThemeManager.createThemeOptionsInSettings(themeOptionsGrid);
    }
});