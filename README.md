# HardWare

پروژهٔ سخت‌افزاری و درس‌های عملی برای انواع میکروکنترلرها و بردهای توسعه.

## میکروکنترلرها و بردها

هر برد در یک پوشهٔ جداگانه قرار دارد. داخل هر پوشه فقط یک `README.md` با مشخصات و **نقشهٔ پین‌ها** همان برد است؛ پروژه‌ها را بعداً به‌صورت فایل در همان پوشه اضافه می‌کنی (بدون زیرپوشهٔ اضافه).

| پوشه | برد |
|------|-----|
| [arduino-uno-r3](./arduino-uno-r3/) | Arduino Uno R3 (ATmega328P) |
| [arduino-nano](./arduino-nano/) | Arduino Nano (ATmega328P) |
| [arduino-pro-mini](./arduino-pro-mini/) | Arduino Pro Mini (3.3V/8MHz یا 5V/16MHz) |
| [esp32](./esp32/) | ESP32 (WiFi + Bluetooth) |
| [teensy-4.1](./teensy-4.1/) | Teensy 4.1 (ARM Cortex-M7, 600 MHz) |

## ساختار کلی

```
HardWare-/
├── README.md
├── arduino-uno-r3/
├── arduino-nano/
├── arduino-pro-mini/
├── esp32/
│   ├── README.md
│   └── hozur-attendance/   # حضور و غیاب RFID (firmware + server + docs)
└── teensy-4.1/
```

پروژه‌های کوچک را مستقیم داخل پوشهٔ هر برد بگذار؛ پروژه‌های بزرگ‌تر (مثل مونوریپو) می‌توانند زیرپوشه داشته باشند.

---
*پروژه HardWare*
