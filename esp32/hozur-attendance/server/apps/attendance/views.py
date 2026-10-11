import hashlib
import hmac
import logging

from django.conf import settings
from rest_framework import status
from rest_framework.response import Response
from rest_framework.throttling import SimpleRateThrottle
from rest_framework.views import APIView

from apps.accounts.models import Person

from .auth import authenticate_reader
from .services import (
    process_enroll,
    process_offline_sync,
    process_scan,
    scan_result_to_dict,
)

logger = logging.getLogger("hozour")


class ScanThrottle(SimpleRateThrottle):
    scope = "scan"

    def get_cache_key(self, request, view):
        ident = request.headers.get("X-Reader-Id") or self.get_ident(request)
        return self.cache_format % {"scope": self.scope, "ident": ident}


class ReaderAPIView(APIView):
    authentication_classes = []
    permission_classes = []
    throttle_classes = [ScanThrottle]

    def initial(self, request, *args, **kwargs):
        super().initial(request, *args, **kwargs)
        self.reader, self.api_key = authenticate_reader(request)


class ScanView(ReaderAPIView):
    def post(self, request):
        data = request.data or {}
        res = process_scan(
            reader=self.reader,
            uid=str(data.get("uid", "")),
            user_id=data.get("user_id"),
            counter=int(data.get("counter") or 0),
            device_time=str(data.get("device_time") or ""),
        )
        http = status.HTTP_200_OK
        if res.status == "denied":
            http = status.HTTP_200_OK  # device still needs structured body
        return Response(scan_result_to_dict(res), status=http)


class EnrollView(ReaderAPIView):
    def post(self, request):
        data = request.data or {}
        result = process_enroll(
            reader=self.reader,
            uid=str(data.get("uid", "")),
            user_id=int(data.get("user_id") or 0),
        )
        code = status.HTTP_200_OK if result.get("status") == "ok" else status.HTTP_400_BAD_REQUEST
        return Response(result, status=code)


class SyncView(ReaderAPIView):
    def get(self, request):
        cards = []
        for p in Person.objects.filter(active=True).exclude(card_uid__isnull=True).exclude(
            card_uid=""
        ):
            cards.append(
                {
                    "uid": p.card_uid,
                    "user_id": p.pk,
                    "active": p.active and not p.blacklisted,
                    "last_counter": p.last_card_counter,
                }
            )
        return Response({"cards": cards})


class OfflineSyncView(ReaderAPIView):
    def post(self, request):
        scans = request.data.get("scans") or []
        if not isinstance(scans, list):
            return Response({"status": "denied", "message": "scans must be list"}, status=400)
        result = process_offline_sync(reader=self.reader, scans=scans)
        return Response(result)


class DeriveKeyView(ReaderAPIView):
    """Optional: server-side diversified KeyA for a UID (disabled by default)."""

    def post(self, request):
        if not settings.HOZUR_ENABLE_KEY_DERIVATION_API:
            return Response({"status": "denied", "message": "disabled"}, status=403)
        master = (settings.HOZUR_MASTER_KEY or "").encode("utf-8")
        if not master:
            return Response({"status": "denied", "message": "no master key"}, status=500)
        uid_hex = str(request.data.get("uid", "")).strip().upper()
        sector = int(request.data.get("sector") or 1)
        try:
            uid = bytes.fromhex(uid_hex)
        except ValueError:
            return Response({"status": "denied", "message": "bad uid"}, status=400)
        msg = uid + bytes([sector & 0xFF])
        digest = hmac.new(master, msg, hashlib.sha256).digest()
        key_a = digest[:6].hex().upper()
        msg_b = uid + bytes([sector & 0xFF, 0x42])
        key_b = hmac.new(master, msg_b, hashlib.sha256).digest()[:6].hex().upper()
        return Response({"status": "ok", "key_a": key_a, "key_b": key_b})
