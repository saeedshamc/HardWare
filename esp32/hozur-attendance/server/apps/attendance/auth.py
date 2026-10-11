import hashlib
import hmac
import logging
import time
from datetime import timedelta

from django.conf import settings
from django.utils import timezone
from rest_framework.exceptions import AuthenticationFailed

from apps.readers.models import Reader

from .models import ApiNonce

logger = logging.getLogger("hozour")


def _get_header(request, name: str) -> str:
    return request.headers.get(name) or request.META.get(
        "HTTP_" + name.upper().replace("-", "_"), ""
    )


def authenticate_reader(request) -> tuple[Reader, str]:
    """
    Validate reader API key + HMAC signature + timestamp/nonce replay window.
    Returns (reader, raw_api_key).
    """
    reader_id = _get_header(request, "X-Reader-Id")
    api_key = _get_header(request, "X-Api-Key")
    timestamp = _get_header(request, "X-Timestamp")
    nonce = _get_header(request, "X-Nonce")
    signature = _get_header(request, "X-Signature")

    if not all([reader_id, api_key, timestamp, nonce, signature]):
        raise AuthenticationFailed("missing auth headers")

    try:
        reader = Reader.objects.get(pk=int(reader_id), active=True)
    except (Reader.DoesNotExist, ValueError) as exc:
        raise AuthenticationFailed("unknown reader") from exc

    if not reader.check_api_key(api_key):
        logger.info("denied bad api key reader=%s", reader_id)
        raise AuthenticationFailed("invalid api key")

    try:
        ts = int(timestamp)
    except ValueError as exc:
        raise AuthenticationFailed("bad timestamp") from exc

    now = int(time.time())
    if abs(now - ts) > settings.HOZUR_REPLAY_WINDOW_SEC:
        raise AuthenticationFailed("timestamp outside replay window")

    if ApiNonce.objects.filter(nonce=nonce).exists():
        raise AuthenticationFailed("replayed nonce")

    body = request.body or b""
    msg = f"{timestamp}.{nonce}.".encode("utf-8") + body
    expected = hmac.new(api_key.encode("utf-8"), msg, hashlib.sha256).hexdigest()
    if not hmac.compare_digest(expected, signature.lower()):
        logger.info("denied bad signature reader=%s", reader_id)
        raise AuthenticationFailed("bad signature")

    ApiNonce.objects.create(nonce=nonce, reader=reader)
    # opportunistic cleanup
    cutoff = timezone.now() - timedelta(seconds=settings.HOZUR_REPLAY_WINDOW_SEC * 2)
    ApiNonce.objects.filter(created_at__lt=cutoff).delete()

    return reader, api_key
