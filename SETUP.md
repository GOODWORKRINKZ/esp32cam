# Быстрая настройка ESP32-CAM Line Detector

## Шаг 1: Подключение ESP32-CAM для прошивки

### Необходимое оборудование:
- ESP32-CAM модуль
- USB-TTL адаптер (FTDI или CH340)
- Провода для соединения

### Схема подключения:

```
ESP32-CAM          USB-TTL
---------          -------
5V        <--->    5V (или VCC)
GND       <--->    GND
U0R (RX)  <--->    TX
U0T (TX)  <--->    RX
IO0       <--->    GND (только при прошивке!)
```

**ВАЖНО:** Подключайте IO0 к GND только во время прошивки. После успешной прошивки отключите этот провод и перезагрузите модуль.

## Шаг 2: Установка PlatformIO

### Вариант A: VS Code (рекомендуется)
1. Установите [Visual Studio Code](https://code.visualstudio.com/)
2. Откройте VS Code
3. Перейдите в Extensions (Ctrl+Shift+X)
4. Найдите "PlatformIO IDE" и установите
5. Перезапустите VS Code

### Вариант B: PlatformIO Core (командная строка)
```bash
# Linux/macOS
curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py -o get-platformio.py
python3 get-platformio.py

# Windows (PowerShell)
# Установите Python 3.6+, затем:
pip install platformio
```

## Шаг 3: Загрузка и открытие проекта

```bash
# Клонируйте репозиторий
git clone https://github.com/GOODWORKRINKZ/esp32cam.git
cd esp32cam

# Откройте в VS Code (если используете VS Code)
code .
```

## Шаг 4: Настройка WiFi (опционально)

По умолчанию ESP32-CAM создает точку доступа WiFi. Если хотите подключить к существующей сети:

Откройте `src/main.cpp` и найдите строки:

```cpp
// WiFi credentials - update these for your network
const char* ssid = "ESP32-CAM-LineDetector";
const char* password = "12345678";
```

### Вариант A: Точка доступа (по умолчанию)
Оставьте как есть. ESP32-CAM создаст собственную WiFi сеть.

### Вариант B: Подключение к существующей сети
Измените функцию `setup()`:

Найдите:
```cpp
// Start WiFi as Access Point
WiFi.softAP(ssid, password);
IPAddress IP = WiFi.softAPIP();
```

Замените на:
```cpp
// Connect to existing WiFi network
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

WiFi.begin(ssid, password);
Serial.print("Connecting to WiFi");
while (WiFi.status() != WL_CONNECTED) {
  delay(500);
  Serial.print(".");
}
Serial.println();
IPAddress IP = WiFi.localIP();
```

## Шаг 5: Сборка и прошивка

### Через VS Code:
1. Откройте проект в VS Code
2. Нажмите на значок PlatformIO в боковой панели (иконка муравья)
3. PROJECT TASKS → esp32cam → General → Build
4. Подключите ESP32-CAM по схеме выше (IO0 к GND!)
5. PROJECT TASKS → esp32cam → General → Upload
6. Отключите IO0 от GND
7. Нажмите кнопку Reset на ESP32-CAM (или отключите/подключите питание)

### Через командную строку:
```bash
# Сборка
pio run

# Прошивка (убедитесь что IO0 подключен к GND!)
pio run --target upload

# После прошивки отключите IO0 от GND и перезагрузите

# Мониторинг Serial порта
pio device monitor
```

## Шаг 6: Первый запуск

1. Отключите IO0 от GND (если был подключен)
2. Перезагрузите ESP32-CAM (кнопка Reset или переподключите питание)
3. Откройте Serial Monitor (115200 baud):
   - VS Code: PlatformIO → Monitor
   - Командная строка: `pio device monitor`

Вы должны увидеть:
```
ESP32-CAM Line Detector Starting...
Camera initialized
AP IP address: 192.168.4.1
Web server started
Connect to WiFi: ESP32-CAM-LineDetector
Open browser at: http://192.168.4.1
```

## Шаг 7: Подключение и использование

1. **На компьютере/телефоне:**
   - Откройте список WiFi сетей
   - Найдите сеть `ESP32-CAM-LineDetector`
   - Подключитесь (пароль: `12345678`)

2. **Откройте браузер:**
   - Перейдите по адресу: `http://192.168.4.1`
   - Вы увидите веб-интерфейс с видеопотоком

3. **Настройте камеру:**
   - Используйте ползунки для настройки параметров
   - Попробуйте предустановленные режимы
   - Наблюдайте за изменениями в реальном времени

## Решение проблем

### Проблема: Не могу прошить ESP32-CAM
**Решение:**
- Убедитесь, что IO0 подключен к GND во время прошивки
- Проверьте правильность подключения RX↔TX (перепутаны местами!)
- Попробуйте нажать кнопку Reset перед началом прошивки
- Проверьте, что выбран правильный COM порт

### Проблема: ESP32-CAM постоянно перезагружается
**Решение:**
- Используйте внешний источник питания 5V/2A (USB часто недостаточно)
- Отключите IO0 от GND после прошивки!
- Добавьте конденсатор 100-470µF между 5V и GND

### Проблема: Камера не инициализируется
**Решение:**
- Проверьте плотность подключения камерного шлейфа
- Убедитесь, что защелка зафиксирована
- Попробуйте переподключить шлейф (синяя сторона к камере)

### Проблема: Не вижу WiFi сеть
**Решение:**
- Откройте Serial Monitor и проверьте сообщения
- Убедитесь, что код успешно инициализировал камеру
- Попробуйте перезагрузить ESP32-CAM
- Проверьте, что ваше устройство поддерживает 2.4GHz WiFi

### Проблема: Медленная трансляция видео
**Решение:**
- Уменьшите разрешение до QVGA (320x240)
- Увеличьте параметр Quality до 15-20
- Убедитесь, что близко к ESP32-CAM (сильный сигнал WiFi)
- Закройте другие приложения, использующие WiFi

## Оптимизация для вашего робота

1. **Начните с режима "Сбалансированный"** (Balanced preset)
2. **Поместите черную линию** на белой поверхности перед камерой
3. **Проверьте контрастность** линии на изображении
4. **Настройте параметры:**
   - Если линия нечеткая: увеличьте Contrast (+1 или +2)
   - Если слишком темно/светло: настройте Brightness
   - Если медленно: уменьшите разрешение или увеличьте Quality

5. **Тестируйте на треке:**
   - Попробуйте режим "В помещении" для искусственного света
   - Попробуйте режим "На улице" для естественного света
   - Настройте под конкретные условия вашего соревнования

## Дополнительные возможности

### Добавление OLED дисплея
Можно подключить OLED дисплей для отображения IP адреса без Serial Monitor:
- SDA → GPIO14
- SCL → GPIO15

### Добавление кнопок управления
Добавьте физические кнопки для переключения предустановок:
- Button 1 → GPIO12 (Preset 1)
- Button 2 → GPIO13 (Preset 2)

### Сохранение настроек
Настройки можно сохранять в EEPROM для автоматической загрузки при включении.

## Полезные ссылки

- [Схемы ESP32-CAM](https://github.com/raphaelbs/esp32-cam-ai-thinker)
- [Документация камеры OV2640](https://www.uctronics.com/download/cam_module/OV2640DS.pdf)
- [PlatformIO для ESP32](https://docs.platformio.org/en/latest/platforms/espressif32.html)

## Успехов в соревнованиях! 🏆🤖