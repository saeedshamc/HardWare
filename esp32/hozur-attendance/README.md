# Hozur (hozur-attendance)

سیستم حضور و غیاب مبتنی بر RFID برای کارکنان یا دانشجویان.

| بخش | مسیر | نقش |
|-----|------|-----|
| فریم‌ویر | [`firmware/`](./firmware/) | ESP32 + RC522 + TFT ILI9341 |
| سرور | [`server/`](./server/) | Django REST + SQLite + Admin |
| مستندات | [`docs/`](./docs/) | سیم‌کشی، پروتکل، راه‌اندازی |

## سخت‌افزار

- ESP32 DevKit + MFRC522 (RC522) + کارت MIFARE Classic 1K
- نمایشگر SPI TFT ILI9341 (240×320)
- بازر + LED وضعیت + دکمه حالت IN/OUT (اختیاری)
- RTC اختیاری: DS3231

## سرور

قابل اجرا روی:

- **کامپیوتر معمولی** (Windows / Linux / macOS) با Docker یا venv
- **برد لینوکسی کوچک** مثل Luckfox Pico (ARM)

جزئیات: [`docs/setup.md`](./docs/setup.md)

## شروع سریع

```bash
# سرور روی PC یا Luckfox (Docker)
cd server
cp .env.example .env
docker compose up --build
# سپس: docker compose exec web python manage.py createsuperuser

# یا بدون Docker روی PC:
# python -m venv .venv && pip install -r requirements.txt
# python manage.py migrate && python manage.py runserver 0.0.0.0:8000

# فریم‌ویر
cd ../firmware
cp include/secrets.example.h include/secrets.h   # مقادیر را پر کنید
pio run -t upload
pio run -t uploadfs   # LittleFS برای صف آفلاین
```

گزارش روزانه ادمین: `/admin/reports/daily/` (خروجی CSV / Excel)

## امنیت

- `MASTER_KEY` و API keyها داخل ریپو نیستند.
- از `secrets.example.h` و `.env.example` کپی بگیرید.
- کلیدهای کارت به‌صورت diversified از HMAC-SHA256 مشتق می‌شوند.
