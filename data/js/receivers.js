// Управление приемниками и детектором уровня

let levelChart = null;
let levelData = {
    labels: [],
    voltages: []
};

// Инициализация при загрузке страницы
document.addEventListener('DOMContentLoaded', function() {
    initLevelChart();
    updateStatus();
    setInterval(updateStatus, 2000); // Обновление каждые 2 секунды
    setInterval(updateLevelChart, 1000); // Обновление графика каждую секунду
});

// Инициализация графика уровня
function initLevelChart() {
    const ctx = document.getElementById('levelChart');
    if (!ctx) return;

    levelChart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [{
                label: 'Напряжение (В)',
                data: [],
                borderColor: 'rgb(75, 192, 192)',
                backgroundColor: 'rgba(75, 192, 192, 0.2)',
                tension: 0.1,
                pointRadius: 0,
                pointHoverRadius: 3
            }, {
                label: 'Порог (В)',
                data: [],
                borderColor: 'rgb(255, 99, 132)',
                borderDash: [5, 5],
                fill: false,
                pointRadius: 0,
                pointHoverRadius: 0
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            animation: false, // Отключаем анимацию для стабильности
            layout: {
                padding: {
                    top: 10,
                    bottom: 10
                }
            },
            interaction: {
                intersect: false,
                mode: 'index'
            },
            scales: {
                x: {
                    display: true,
                    ticks: {
                        maxTicksLimit: 10 // Ограничиваем количество меток
                    }
                },
                y: {
                    beginAtZero: true,
                    max: 5.0,
                    min: 0
                }
            },
            plugins: {
                legend: {
                    display: true
                },
                tooltip: {
                    enabled: true
                }
            }
        }
    });
}

// Обновление статуса приемников и детектора уровня
async function updateStatus() {
    try {
        // Получаем статус приемников
        const receiversResponse = await fetch('/api/receivers/status');
        if (receiversResponse.ok) {
            const receiversData = await receiversResponse.json();
            updateReceiversDisplay(receiversData);
        }

        // Получаем статус детектора уровня
        const levelResponse = await fetch('/api/receivers/level/status');
        if (levelResponse.ok) {
            const levelData = await levelResponse.json();
            updateLevelDisplay(levelData);
        }
    } catch (error) {
        console.error('Ошибка обновления статуса:', error);
    }
}

// Обновление отображения приемников (оптимизированная версия)
function updateReceiversDisplay(data) {
    const container = document.getElementById('receiversStatus');
    if (!container) return;

    if (!data.receivers || !Array.isArray(data.receivers)) {
        if (container.children.length === 0) {
            container.innerHTML = '<p>Нет данных о приемниках</p>';
        }
        return;
    }

    // Обновляем существующие карточки или создаем новые
    data.receivers.forEach((receiver, index) => {
        const receiverId = `receiver-${receiver.id}`;
        let card = document.getElementById(receiverId);
        
        const isActive = receiver.active;
        const isOverflowing = receiver.overflowing;
        const volumePercent = (receiver.current_volume / receiver.max_volume) * 100;
        
        if (!card) {
            // Создаем новую карточку
            card = document.createElement('div');
            card.id = receiverId;
            card.className = `receiver-card ${isActive ? 'active' : ''} ${isOverflowing ? 'overflowing' : ''}`;
            container.appendChild(card);
        }
        
        // Обновляем только измененные данные, не пересоздавая весь DOM
        const header = card.querySelector('.receiver-header');
        const body = card.querySelector('.receiver-body');
        
        if (!header || !body) {
            // Если структуры нет, создаем заново
            card.innerHTML = `
                <div class="receiver-header">
                    <h3>${receiver.name || `Приемник ${receiver.id + 1}`}</h3>
                    <span class="badge-container"></span>
                </div>
                <div class="receiver-body">
                    <div class="receiver-info">
                        <div class="info-row">
                            <span class="info-label">Объем:</span>
                            <span class="info-value">${receiver.current_volume.toFixed(1)} / ${receiver.max_volume} мл</span>
                        </div>
                        <div class="info-row">
                            <span class="info-label">Фракция:</span>
                            <span class="info-value">${receiver.fraction || 'Не указана'}</span>
                        </div>
                        <div class="progress-bar">
                            <div class="progress-fill" style="width: ${Math.min(volumePercent, 100)}%"></div>
                        </div>
                    </div>
                </div>
            `;
        } else {
            // Обновляем только измененные элементы
            const nameEl = header.querySelector('h3');
            if (nameEl) nameEl.textContent = receiver.name || `Приемник ${receiver.id + 1}`;
            
            const badgeContainer = header.querySelector('.badge-container') || header;
            const existingBadges = badgeContainer.querySelectorAll('.badge');
            existingBadges.forEach(b => b.remove());
            
            if (isActive) {
                const badge = document.createElement('span');
                badge.className = 'badge badge-success';
                badge.textContent = 'АКТИВЕН';
                badgeContainer.appendChild(badge);
            }
            if (isOverflowing) {
                const badge = document.createElement('span');
                badge.className = 'badge badge-danger';
                badge.textContent = 'ПЕРЕПОЛНЕНИЕ';
                badgeContainer.appendChild(badge);
            }
            
            // Обновляем классы карточки
            card.className = `receiver-card ${isActive ? 'active' : ''} ${isOverflowing ? 'overflowing' : ''}`;
            
            // Обновляем значения
            const volumeValue = body.querySelector('.info-row .info-value');
            if (volumeValue) {
                volumeValue.textContent = `${receiver.current_volume.toFixed(1)} / ${receiver.max_volume} мл`;
            }
            
            const fractionValue = body.querySelectorAll('.info-row .info-value')[1];
            if (fractionValue) {
                fractionValue.textContent = receiver.fraction || 'Не указана';
            }
            
            // Обновляем прогресс-бар
            const progressFill = body.querySelector('.progress-fill');
            if (progressFill) {
                progressFill.style.width = `${Math.min(volumePercent, 100)}%`;
            }
        }
    });
    
    // Удаляем лишние карточки, если их стало меньше
    const existingCards = container.querySelectorAll('.receiver-card');
    existingCards.forEach((card, index) => {
        if (index >= data.receivers.length) {
            card.remove();
        }
    });

    // Обновляем настройки авто-переключения
    if (document.getElementById('autoSwitchEnabled')) {
        document.getElementById('autoSwitchEnabled').checked = data.auto_switch || false;
    }

    // Обновляем действие при переполнении
    if (data.overflow_action) {
        const actionSelect = document.getElementById('overflowAction');
        if (actionSelect) {
            actionSelect.value = data.overflow_action;
        }
    }
}

// Обновление отображения детектора уровня
function updateLevelDisplay(data) {
    if (!data) return;

    // Статус
    const levelBadge = document.getElementById('levelBadge');
    if (levelBadge) {
        levelBadge.textContent = data.enabled ? 'Включен' : 'Выключен';
        levelBadge.className = 'badge ' + (data.enabled ? 'badge-success' : 'badge-secondary');
    }

    // Напряжение
    const levelVoltage = document.getElementById('levelVoltage');
    if (levelVoltage && data.voltage !== undefined) {
        levelVoltage.textContent = data.voltage.toFixed(3) + ' V';
    }

    // Порог
    const levelThreshold = document.getElementById('levelThreshold');
    if (levelThreshold && data.threshold !== undefined) {
        levelThreshold.textContent = data.threshold.toFixed(3) + ' V';
        
        // Обновляем поле ввода
        const thresholdInput = document.getElementById('thresholdInput');
        if (thresholdInput) {
            thresholdInput.value = data.threshold;
        }
    }

    // Переполнение
    const overflowBadge = document.getElementById('overflowBadge');
    const normalBadge = document.getElementById('normalBadge');
    if (data.overflow) {
        if (overflowBadge) overflowBadge.style.display = 'inline-block';
        if (normalBadge) normalBadge.style.display = 'none';
    } else {
        if (overflowBadge) overflowBadge.style.display = 'none';
        if (normalBadge) normalBadge.style.display = 'inline-block';
    }
}

// Обновление графика уровня
async function updateLevelChart() {
    try {
        const response = await fetch('/api/receivers/level/voltage');
        if (response.ok) {
            const data = await response.json();
            const voltage = data.voltage || 0;

            // Получаем порог
            const statusResponse = await fetch('/api/receivers/level/status');
            let threshold = 0.5;
            if (statusResponse.ok) {
                const statusData = await statusResponse.json();
                threshold = statusData.threshold || 0.5;
            }

            // Добавляем данные в график
            const now = new Date().toLocaleTimeString();
            levelData.labels.push(now);
            levelData.voltages.push(voltage);

            // Ограничиваем количество точек (последние 60)
            if (levelData.labels.length > 60) {
                levelData.labels.shift();
                levelData.voltages.shift();
            }

            // Обновляем график
            if (levelChart) {
                // Обновляем данные напрямую, без пересоздания массивов
                levelChart.data.labels = [...levelData.labels];
                levelChart.data.datasets[0].data = [...levelData.voltages];
                
                // Обновляем линию порога (создаем массив один раз)
                levelChart.data.datasets[1].data = levelData.labels.map(() => threshold);
                
                // Обновляем без анимации и без пересчета масштаба
                levelChart.update('none');
            }
        }
    } catch (error) {
        console.error('Ошибка обновления графика:', error);
    }
}

// Установка порога
async function setThreshold() {
    const input = document.getElementById('thresholdInput');
    if (!input) return;

    const threshold = parseFloat(input.value);
    if (isNaN(threshold) || threshold < 0 || threshold > 5) {
        alert('Некорректное значение порога (0-5 В)');
        return;
    }

    try {
        const response = await fetch('/api/receivers/level/threshold', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ threshold: threshold })
        });

        if (response.ok) {
            alert('Порог успешно установлен');
            updateStatus();
        } else {
            alert('Ошибка установки порога');
        }
    } catch (error) {
        console.error('Ошибка установки порога:', error);
        alert('Ошибка установки порога');
    }
}

// Калибровка пустого приемника
async function calibrateEmpty() {
    try {
        const response = await fetch('/api/receivers/level/calibrate/empty', {
            method: 'POST'
        });

        if (response.ok) {
            alert('Калибровка пустого приемника выполнена');
            updateStatus();
        } else {
            alert('Ошибка калибровки');
        }
    } catch (error) {
        console.error('Ошибка калибровки:', error);
        alert('Ошибка калибровки');
    }
}

// Калибровка полного приемника
async function calibrateFull() {
    try {
        const response = await fetch('/api/receivers/level/calibrate/full', {
            method: 'POST'
        });

        if (response.ok) {
            alert('Калибровка полного приемника выполнена');
            updateStatus();
        } else {
            alert('Ошибка калибровки');
        }
    } catch (error) {
        console.error('Ошибка калибровки:', error);
        alert('Ошибка калибровки');
    }
}

// Переключение приемника
async function switchReceiver(receiverId) {
    try {
        const response = await fetch('/api/receivers/switch', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ receiver_id: receiverId })
        });

        if (response.ok) {
            alert(`Переключено на приемник ${receiverId + 1}`);
            updateStatus();
        } else {
            alert('Ошибка переключения приемника');
        }
    } catch (error) {
        console.error('Ошибка переключения:', error);
        alert('Ошибка переключения приемника');
    }
}

// Установка действия при переполнении
async function setOverflowAction() {
    const select = document.getElementById('overflowAction');
    if (!select) return;

    const action = select.value;
    try {
        const response = await fetch('/api/receivers/overflow/action', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ action: action })
        });

        if (response.ok) {
            // Успешно
        } else {
            alert('Ошибка установки действия при переполнении');
        }
    } catch (error) {
        console.error('Ошибка установки действия:', error);
        alert('Ошибка установки действия при переполнении');
    }
}

// Установка авто-переключения
async function setAutoSwitch() {
    const checkbox = document.getElementById('autoSwitchEnabled');
    if (!checkbox) return;

    const enabled = checkbox.checked;
    try {
        const response = await fetch('/api/receivers/auto-switch', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ enabled: enabled })
        });

        if (response.ok) {
            // Успешно
        } else {
            alert('Ошибка установки авто-переключения');
            checkbox.checked = !enabled; // Откатываем изменение
        }
    } catch (error) {
        console.error('Ошибка установки авто-переключения:', error);
        alert('Ошибка установки авто-переключения');
        checkbox.checked = !enabled; // Откатываем изменение
    }
}

// Загрузка конфигурации приемников
async function loadReceiverConfig() {
    try {
        const response = await fetch('/api/receivers/config');
        if (response.ok) {
            const data = await response.json();
            displayReceiverConfig(data);
        } else {
            alert('Ошибка загрузки конфигурации');
        }
    } catch (error) {
        console.error('Ошибка загрузки конфигурации:', error);
        alert('Ошибка загрузки конфигурации');
    }
}

// Отображение конфигурации приемников
function displayReceiverConfig(data) {
    const container = document.getElementById('receiverConfig');
    if (!container || !data.receivers) return;

    container.innerHTML = data.receivers.map(receiver => {
        return `
            <div class="form-group">
                <label>Приемник ${receiver.id + 1} (${receiver.name || 'Не указано'})</label>
                <div class="input-group">
                    <input type="text" id="receiver_${receiver.id}_name" class="form-control" 
                           placeholder="Название" value="${receiver.name || ''}">
                    <input type="number" id="receiver_${receiver.id}_gpio" class="form-control" 
                           placeholder="GPIO пин" value="${receiver.gpio_pin || ''}" min="0" max="50">
                    <input type="number" id="receiver_${receiver.id}_max_volume" class="form-control" 
                           placeholder="Макс. объем (мл)" value="${receiver.max_volume || ''}" min="0" step="0.1">
                </div>
            </div>
        `;
    }).join('');
}

// Сохранение конфигурации приемников
async function saveReceiverConfig() {
    try {
        const receivers = [];
        for (let i = 0; i < 3; i++) {
            const nameInput = document.getElementById(`receiver_${i}_name`);
            const gpioInput = document.getElementById(`receiver_${i}_gpio`);
            const maxVolumeInput = document.getElementById(`receiver_${i}_max_volume`);
            
            if (nameInput && gpioInput && maxVolumeInput) {
                receivers.push({
                    id: i,
                    name: nameInput.value || `Приемник ${i + 1}`,
                    gpio_pin: parseInt(gpioInput.value) || 0,
                    max_volume: parseFloat(maxVolumeInput.value) || 1000.0
                });
            }
        }

        const response = await fetch('/api/receivers/config', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ receivers: receivers })
        });

        if (response.ok) {
            alert('Конфигурация успешно сохранена');
            updateStatus(); // Обновляем статус
        } else {
            alert('Ошибка сохранения конфигурации');
        }
    } catch (error) {
        console.error('Ошибка сохранения конфигурации:', error);
        alert('Ошибка сохранения конфигурации');
    }
}

// Загружаем конфигурацию при загрузке страницы
document.addEventListener('DOMContentLoaded', function() {
    loadReceiverConfig();
});
