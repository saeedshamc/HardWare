# راه‌اندازی Hozur

## پیش‌نیازها

- Python 3.11+ (برای اجرای بدون Docker)
- Docker + Docker Compose (پیشنهادی)
- PlatformIO Core یا VS Code/Cursor + افزونه PlatformIO
- شبکهٔ LAN مشترک بین ESP32 و سرور

---

## مسیر A — کامپیوتر معمولی (PC)

### با Docker (پیشنهادی)

```bash
cd esp32/hozur-attendance/server
cp .env.example .env
# مقادیر SECRET_KEY، ALLOWED_HOSTS، HOZUR_MASTER_KEY را ویرایش کنید
docker compose up --build
```

- API: `http://<IP-PC>:8000/api/`
- Admin: `http://<IP-PC>:8000/admin/`

ساخت ادمین:

```bash
docker compose exec web python manage.py createsuperuser
```

### بدون Docker (venv)

```bash
cd esp32/hozur-attendance/server
python -m venv .venv

# Windows
.venv\Scripts\activate
# Linux/macOS
source .venv/bin/activate

pip install -r requirements.txt
cp .env.example .env
python manage.py migrate
python manage.py createsuperuser
python manage.py runserver 0.0.0.0:8000
```

فونت Vazirmatn را در `server/fonts/Vazirmatn-Regular.ttf` قرار دهید (لینک در README سرور).

---

## مسیر B — Luckfox Pico / برد ARM لینوکسی

1. Docker را روی برد نصب کنید (یا از ایمیج رسمی برد استفاده کنید).
2. همان مراحل Docker مسیر A را اجرا کنید؛ ایمیج `python:3.11-slim` روی ARM64/ARM32 با wheelهای مناسب ساخته می‌شود.
3. اگر build روی خود برد کند است، روی PC با buildx بسازید و push کنید:

```bash
docker buildx build --platform linux/arm64 -t hozur-server:arm64 --load .
```

4. SQLite و media روی volume بمانند تا با ریبوت از بین نروند.

حداقل منابع پیشنهادی: ~256MB RAM آزاد برای Django + gunicorn با یک worker.

---

## فریم‌ویر ESP32

```bash
cd esp32/hozur-attendance/firmware
cp include/secrets.example.h include/secrets.h
```

در `secrets.h` این‌ها را پر کنید:

- `WIFI_SSID` / `WIFI_PASSWORD`
- `SERVER_BASE_URL` (مثلاً `http://192.168.1.50:8000`)
- `READER_ID` / `READER_API_KEY`
- `MASTER_KEY` (۱۶+ بایت تصادفی؛ باید با سرور یکی باشد اگر derivation سمت سرور هم فعال است)

```bash
pio run -t upload
pio device monitor
```

Enrollment سریال:

```
ENROLL 42
```

سپس کارت را نزدیک خواننده بگیرید. با `ENROLL_DRY_RUN=1` فقط شبیه‌سازی می‌شود.

---

## فعال‌سازی Flash Encryption (ESP32) — خلاصه

برای محافظت از `MASTER_KEY` روی فلش:

1. در `menuconfig` / sdkconfig: Flash Encryption → Development یا Release mode.
2. NVS Encryption را فعال کنید تا کلید در NVS رمز شود.
3. بعد از enable در حالت Release، فلش معمولی دیگر خوانا نیست — فقط با کلید امنیتی خودتان.
4. جزئیات رسمی: [Espressif Flash Encryption](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/security/flash-encryption.html)

برای توسعهٔ اولیه، `secrets.h` + عدم commit کافی است؛ برای استقرار واقعی Flash Encryption توصیه می‌شود.

---

## چک‌لیست اولیه‌سازی سرور

1. یک `Reader` در ادمین بسازید و `api_key` را در ESP32 بگذارید.
2. کاربران را با `user_id` مشخص ثبت کنید.
3. کارت را enroll کنید (`ENROLL <user_id>` یا API `/api/enroll/`).
4. یک اسکن آزمایشی بزنید و لاگ را در ادمین ببینید.
