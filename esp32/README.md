# ESP32

بخش پروژه‌های مرتبط با **ESP32**.

## درباره برد

ESP32 یک SoC دو هسته‌ای با وای‌فای و بلوتوث است؛ مناسب پروژه‌های IoT، وب سرور، ارتباط بی‌سیم و سنسورها.

## مشخصات کلی

| مورد | مقدار |
|------|--------|
| **تراشه** | ESP32 (Xtensa dual-core 32-bit LX6) |
| **فرکانس** | تا ۲۴۰ مگاهرتز |
| **حافظه فلش** | معمولاً ۴ مگابایت (بسته به ماژول) |
| **SRAM** | ۵۲۰ کیلوبایت |
| **ولتاژ کاری** | ۳٫۰–۳٫۶ ولت |
| **وای‌فای** | 802.11 b/g/n تا ۱۵۰ Mbps |
| **بلوتوث** | BR/EDR و BLE (Bluetooth 4.2) |

---

## نقشه پین‌ها (Pinout)

منطبق بر بردهای **ESP32 DevKit V1** (DOIT / مشابه). شماره‌گذاری پین روی برد با شماره GPIO در کد یکی نیست؛ در برنامه از **شماره GPIO** استفاده کن.

### هدر سمت چپ (۳۰ پین)

| پین روی برد | GPIO | توضیح | توجه |
|-------------|------|--------|------|
| 3V3 | — | خروجی ۳٫۳V | |
| EN | EN | Enable / ریست (فعال با HIGH) | |
| VP | GPIO 36 | ورودی فقط — ADC1_CH0 | ورودی فقط، بدون pull-up |
| VN | GPIO 39 | ورودی فقط — ADC1_CH3 | ورودی فقط، بدون pull-up |
| 34 | GPIO 34 | ورودی فقط — ADC1_CH6 | ورودی فقط |
| 35 | GPIO 35 | ورودی فقط — ADC1_CH7 | ورودی فقط |
| 32 | GPIO 32 | ADC1_CH4، DAC، Touch | |
| 33 | GPIO 33 | ADC1_CH5، DAC، Touch | |
| 25 | GPIO 25 | ADC2_CH8، DAC، DAC1 | وقتی WiFi فعال است ADC2 استفاده نکن |
| 26 | GPIO 26 | ADC2_CH9، DAC، DAC2 | |
| 27 | GPIO 27 | ADC2_CH7، Touch | |
| 14 | GPIO 14 | ADC2_CH6، Touch | Strapping |
| 12 | GPIO 12 | ADC2_CH5، Touch | Strapping — در بوت HIGH نکن |
| 13 | GPIO 13 | ADC2_CH4، Touch | |
| GND | GND | زمین | |
| VIN | VIN | ورودی ۵V (از USB یا خارجی) | |

### هدر سمت راست

| پین روی برد | GPIO | توضیح | توجه |
|-------------|------|--------|------|
| 15 | GPIO 15 | ADC2_CH3، Touch | Strapping |
| 2 | GPIO 2 | ADC2_CH2، Touch | LED روی برد؛ Strapping |
| 0 | GPIO 0 | ADC2_CH1 | Strapping — برای بوت فلش |
| 4 | GPIO 4 | ADC2_CH0 | |
| 16 | GPIO 16 | — | |
| 17 | GPIO 17 | — | |
| 5 | GPIO 5 | VSPI SS، ADC2_CH0 | Strapping |
| 18 | GPIO 18 | VSPI SCK | |
| 19 | GPIO 19 | VSPI MISO | |
| 21 | GPIO 21 | I2C SDA | |
| 3 | GPIO 3 | UART RX (USB) | در بوت HIGH |
| 1 | GPIO 1 | UART TX (USB)، خروجی دیباگ بوت | |
| 22 | GPIO 22 | I2C SCL | |
| 23 | GPIO 23 | VSPI MOSI | |
| 9 | GPIO 9 | — | ممکن است به فلش وصل باشد |
| 10 | GPIO 10 | — | ممکن است به فلش وصل باشد |
| 11 | GPIO 11 | — | ممکن است به فلش وصل باشد |
| 6–11 | GPIO 6–11 | — | **استفاده نکن** — به فلش داخلی وصل است |
| GND | GND | زمین | |
| 5V | 5V | خروجی ۵V (از رگولاتور) | |

### پین‌های ممنوع یا محدود

| GPIO | دلیل |
|------|------|
| **6, 7, 8, 9, 10, 11** | به فلش SPI داخلی وصل است — استفاده نکن |
| **34, 35, 36, 39** | فقط ورودی؛ بدون pull-up داخلی |
| **12** | در بوت اگر HIGH باشد ممکن است بوت ناموفق شود |
| **0, 2, 5, 12, 15** | Strapping — سطح منطقی در بوت روی حالت بوت اثر می‌گذارد |

### خلاصه کاربرد پین‌ها

| کاربرد | پین‌ها (GPIO) |
|--------|----------------|
| **ADC1** (با WiFi هم کار می‌کند) | 32, 33, 34, 35, 36, 39 |
| **ADC2** (بدون WiFi) | 0, 2, 4, 12, 13, 14, 15, 25, 26, 27 |
| **DAC** | 25, 26 |
| **I2C پیش‌فرض** | 21 (SDA), 22 (SCL) |
| **VSPI** | 18 (SCK), 19 (MISO), 23 (MOSI), 5 (SS) |
| **HSPI** | 14 (SCK), 12 (MISO), 13 (MOSI), 15 (SS) |
| **Touch** | 0, 2, 4, 12, 13, 14, 15, 27, 32, 33 |
| **UART0 (USB)** | 1 (TX), 3 (RX) |

---

## محیط توسعه

- [Arduino IDE](https://www.arduino.cc/en/software) + [بورد ESP32 از Espressif](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html)
- [PlatformIO](https://platformio.org/) با پلتفرم `espressif32`
- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/) برای توسعهٔ حرفه‌ای

---
*بخش ESP32 — پروژه HardWare*
