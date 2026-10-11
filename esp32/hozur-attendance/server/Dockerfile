# Multi-arch friendly (amd64 PC + arm64/arm32 Luckfox)
FROM python:3.11-slim

ENV PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1 \
    PIP_NO_CACHE_DIR=1

WORKDIR /app

RUN apt-get update && apt-get install -y --no-install-recommends \
      libjpeg62-turbo zlib1g \
    && rm -rf /var/lib/apt/lists/*

COPY requirements.txt .
RUN pip install -r requirements.txt

COPY . .

RUN chmod +x /app/docker/entrypoint.sh \
    && mkdir -p /app/data /app/media /app/staticfiles

ENV DATABASE_URL=sqlite:////app/data/db.sqlite3 \
    DJANGO_SETTINGS_MODULE=config.settings

EXPOSE 8000

ENTRYPOINT ["/app/docker/entrypoint.sh"]
CMD ["gunicorn", "config.wsgi:application", "--bind", "0.0.0.0:8000", "--workers", "1", "--threads", "2", "--timeout", "60"]
