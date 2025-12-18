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
                tension: 0.1
            }, {
                label: 'Порог (В)',
                data: [],
                borderColor: 'rgb(255, 99, 132)',
                borderDash: [5, 5],
                fill: false
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            scales: {
                y: {
                    beginAtZero: true,
                    max: 5.0
                }
            },
            plugins: {
                legend: {
                    display: true
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

// Обновление отображения приемников
function updateReceiversDisplay(data) {
    const container = document.getElementById('receiversStatus');
    if (!container) return;

    if (!data.receivers || !Array.isArray(data.receivers)) {
        container.innerHTML = '<p>Нет данных о приемниках</p>';
        return;
    }

    container.innerHTML = data.receivers.map(receiver => {
        const isActive = receiver.active;
        const isOverflowing = receiver.overflowing;
        const volumePercent = (receiver.current_volume / receiver.max_volume) * 100;
        
        return `
            <div class="receiver-card ${isActive ? 'active' : ''} ${isOverflowing ? 'overflowing' : ''}">
                <div class="receiver-header">
                    <h3>${receiver.name || `Приемник ${receiver.id + 1}`}</h3>
                    ${isActive ? '<span class="badge badge-success">АКТИВЕН</span>' : ''}
                    ${isOverflowing ? '<span class="badge badge-danger">ПЕРЕПОЛНЕНИЕ</span>' : ''}
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
            </div>
        `;
    }).join('');

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
                levelChart.data.labels = levelData.labels;
                levelChart.data.datasets[0].data = levelData.voltages;
                
                // Обновляем линию порога
                levelChart.data.datasets[1].data = levelData.labels.map(() => threshold);
                
                levelChart.update('none'); // 'none' для плавной анимации
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

    // TODO: Добавить API endpoint для установки авто-переключения
    // Пока оставляем только UI
}
