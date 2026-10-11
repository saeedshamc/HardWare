#!/bin/sh
set -e
cd /app
python manage.py migrate --noinput
python manage.py collectstatic --noinput
exec "$@"
