# Hozur Server (Django + DRF)

قابل اجرا روی **کامپیوتر معمولی (PC)** و **برد ARM مثل Luckfox Pico**.

## مسیر سریع — PC با Docker

```bash
cp .env.example .env
# SECRET_KEY و HOZUR_MASTER_KEY را عوض کنید
docker compose up --build
```

- Admin: http://127.0.0.1:8000/admin/
- گزارش روزانه: http://127.0.0.1:8000/admin/reports/daily/
- API: http://127.0.0.1:8000/api/scan/

```bash
docker compose exec web python manage.py createsuperuser
```

## مسیر سریع — PC بدون Docker

```bash
cp .env.example .env
python -m venv .venv
# Windows: .venv\Scripts\activate
pip install -r requirements.txt
python manage.py migrate
python manage.py createsuperuser
python manage.py runserver 0.0.0.0:8000
```

## Luckfox / ARM

همان `docker compose up --build` روی برد. ایمیج `python:3.11-slim` روی arm64/armv7 ساخته می‌شود.

برای build روی PC و اجرا روی برد:

```bash
docker buildx build --platform linux/arm64 -t hozur-server:arm64 --load .
```

جزئیات بیشتر: [`../docs/setup.md`](../docs/setup.md)

## تست

```bash
python manage.py test apps.attendance.tests apps.reports.tests
```

## مدل‌ها

- `Person` — کاربر/کارت (به‌جای تداخل با `auth.User`)
- `Reader` — خواننده + هش API key
- `AttendanceLog` — لاگ حضور

فونت فارسی bitmap: `fonts/Vazirmatn-Regular.ttf`
