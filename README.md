# ESP8266 NodeMCU + ENC28J60 Barrier Controller (NetPing IO v2/v4 compatible mode)

Проект для PlatformIO/Arduino под **ESP8266 NodeMCU (ESP-12E)** с Ethernet-модулем **ENC28J60**.

## 1) Схема подключения

### ENC28J60 (SPI)
- SCK  -> D5 (GPIO14)
- MISO -> D6 (GPIO12)
- MOSI -> D7 (GPIO13)
- CS   -> D2 (GPIO4)
- INT/CLK/WOL/RST не используются в коде.

### Relay
- Relay IN1 -> D0 (GPIO16)

### Концевики
- OPEN_LIMIT  -> D1 (GPIO5)
- CLOSE_LIMIT -> RX (GPIO3)

Схема входов концевиков: `GPIO ---- switch ---- GND`, режим `INPUT_PULLUP`.

## 2) Логика работы

Состояния state machine:
- BOOT
- IDLE_UNKNOWN
- IDLE_CLOSED
- OPENING
- OPENED_HOLD
- CLOSING
- ERROR

Поддержаны режимы открытия:
- `PULSE`
- `HOLD_UNTIL_LIMIT`

Все тайминги в конфиге (LittleFS), работа без `delay()` в основной логике.

## 3) NetPing-совместимость

Endpoint: `/io.cgi`

Поддержано:
- `/io.cgi?io1=0`
- `/io.cgi?io1=1`
- `/io.cgi?io1=f`
- `/io.cgi?io1=f,5`
- `/io.cgi?io1`
- `/io.cgi?io1&mode=0`
- `/io.cgi?io1&mode=1`
- `/io.cgi?io1&mode=2`

Ответы:
- успех: `io_result('ok')`
- ошибка: `io_result('error')`
- состояние: `io_result('ok', -1, STATE, COUNTER)`

> Примечание: это максимально близкая совместимость без полного reverse engineering всех прошивок NetPing.

## 4) JSON API

- `GET /api/status`
- `POST /api/open`
- `POST /api/close`
- `POST /api/reset_error`
- `GET /api/config`
- `POST /api/config`
- `POST /api/reboot`

Дополнительно:
- `/event?token=...&action=open`

## 5) Первая прошивка (PlatformIO)

1. Открыть папку проекта в VS Code + PlatformIO.
2. Подключить NodeMCU.
3. Выполнить:
   - `pio run`
   - `pio run -t upload`
4. (Опционально) загрузить LittleFS образ:
   - `pio run -t buildfs`
   - `pio run -t uploadfs`

## 6) Первый вход в web UI

- Открыть IP устройства (DHCP или static из config).
- Страница логина: `/login`
- Дефолт: `admin / admin`

## 7) Где настраиваются login/password

- В разделе **SECURITY** web UI.
- Пароль хранится в `LittleFS` как SHA1 hash (`password_hash`), не открытым текстом.

## 8) Ограничения ESP8266 + ENC28J60

- Небольшой RAM: UI/JSON сделаны компактными.
- ENC28J60 медленнее встроенного MAC/PHY.
- Проект рассчитан на небольшой поток HTTP запросов.
- Важна стабильная 3.3V линия питания и короткие SPI линии.

## 9) Что стоит переписать при переносе на ESP32

- Сетевой слой (использовать нативный Ethernet/Wi-Fi стек ESP32).
- Более безопасная auth/session модель.
- Улучшить web UI (шаблоны + JS) при наличии памяти.
- Вынести API и UI роутинг в более структурированный слой.
