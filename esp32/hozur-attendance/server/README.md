# Hozur Server (Django + DRF)

اجرا روی **PC** یا **Luckfox / ARM**. جزئیات: [`../docs/setup.md`](../docs/setup.md)

```bash
cp .env.example .env
python -m venv .venv
# Windows: .venv\Scripts\activate
pip install -r requirements.txt
python manage.py migrate
python manage.py createsuperuser
python manage.py runserver 0.0.0.0:8000
```

تست‌ها:

```bash
python manage.py test apps.attendance.tests
```

مدل کاربر حضور: `Person` (برای جلوگیری از تداخل با `django.contrib.auth.User`).
