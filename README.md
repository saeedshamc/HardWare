# HardWare

مونوریپوی سخت‌افزار و پروژه‌های عملی برای میکروکنترلرها و بردهای توسعه.

## میکروکنترلرها و بردها

هر برد پوشهٔ خودش را دارد؛ مشخصات و نقشهٔ پین در `README.md` همان پوشه است.

| پوشه | برد |
|------|-----|
| [arduino-uno-r3](./arduino-uno-r3/) | Arduino Uno R3 (ATmega328P) |
| [arduino-nano](./arduino-nano/) | Arduino Nano (ATmega328P) |
| [arduino-pro-mini](./arduino-pro-mini/) | Arduino Pro Mini (3.3V/8MHz یا 5V/16MHz) |
| [esp32](./esp32/) | ESP32 (WiFi + Bluetooth) |
| [teensy-4.1](./teensy-4.1/) | Teensy 4.1 (ARM Cortex-M7، ۶۰۰ MHz) |

## پروژه‌ها

### ESP32

| پوشه | توضیح |
|------|--------|
| [esp32/hozur-attendance](./esp32/hozur-attendance/) | حضور و غیاب RFID: فریم‌ویر ESP32 + سرور Django |
| [esp32/MifareBreaker](./esp32/MifareBreaker/) | فریم‌ویر آزمایشگاهی ESP32 + RC522: دیکشنری، داشبورد وب، سریال JSON، دامپ SD |
| [esp32/MifareKeyLab](./esp32/MifareKeyLab/) | آزمایشگاه آموزشی کلید کاندید (CLI پایتون + فریم‌ویر ESP32) |
| [esp32/MifareNestedAttack](./esp32/MifareNestedAttack/) | آزمایشگاه آموزشی مفاهیم MIFARE Classic (شبیه‌ساز، نه حملهٔ عملیاتی) |
| [esp32/ESP32_CarGame](./esp32/ESP32_CarGame/) | بازی ماشین روی ESP32 |
| [esp32/ESP32_ShieldGame](./esp32/ESP32_ShieldGame/) | بازی شیلد روی ESP32 |

جزئیات برد ESP32 و جدول پروژه‌ها: [esp32/README.md](./esp32/README.md).

### Arduino Pro Mini

| پوشه | توضیح |
|------|--------|
| [arduino-pro-mini/space-shooter-tft](./arduino-pro-mini/space-shooter-tft/) | بازی space-shooter روی TFT |

## ساختار کلی

```
HardWare/
├── README.md
├── arduino-uno-r3/
├── arduino-nano/
├── arduino-pro-mini/
│   └── space-shooter-tft/
├── esp32/
│   ├── README.md
│   ├── hozur-attendance/
│   ├── MifareBreaker/
│   ├── MifareKeyLab/
│   ├── MifareNestedAttack/
│   ├── ESP32_CarGame/
│   └── ESP32_ShieldGame/
└── teensy-4.1/
```

پروژه‌های کوچک را مستقیم داخل پوشهٔ برد بگذار؛ پروژه‌های بزرگ‌تر می‌توانند زیرپوشه داشته باشند.

## استفاده مجاز

ابزارهای RFID این مخزن فقط برای کارت‌ها و سخت‌افزاری است که مال خودت است یا مجوز صریح تست داری. استفادهٔ غیرمجاز غیرقانونی است.

---
*پروژه HardWare*
