from django.db import models

from apps.accounts.models import Person
from apps.readers.models import Reader


class AttendanceLog(models.Model):
    class Direction(models.TextChoices):
        IN = "in", "ورود"
        OUT = "out", "خروج"

    class Status(models.TextChoices):
        OK = "ok", "موفق"
        DENIED = "denied", "رد شده"
        DUPLICATE = "duplicate", "تکراری"
        CLONE_SUSPECT = "clone_suspect", "مظنون کلون"

    user = models.ForeignKey(
        Person,
        on_delete=models.SET_NULL,
        null=True,
        blank=True,
        related_name="logs",
        verbose_name="کاربر",
    )
    reader = models.ForeignKey(
        Reader,
        on_delete=models.PROTECT,
        related_name="logs",
        verbose_name="خواننده",
    )
    direction = models.CharField(
        "جهت", max_length=8, choices=Direction.choices, blank=True
    )
    timestamp = models.DateTimeField("زمان", db_index=True)
    card_uid = models.CharField("UID", max_length=32, blank=True)
    card_counter = models.PositiveIntegerField("شمارنده کارت", default=0)
    status = models.CharField(
        "وضعیت", max_length=20, choices=Status.choices, default=Status.OK
    )
    message = models.CharField("پیام", max_length=255, blank=True)
    device_time = models.CharField("زمان دستگاه", max_length=40, blank=True)
    created_at = models.DateTimeField(auto_now_add=True)

    class Meta:
        verbose_name = "لاگ حضور"
        verbose_name_plural = "لاگ‌های حضور"
        ordering = ["-timestamp"]
        indexes = [
            models.Index(fields=["card_uid", "-timestamp"]),
            models.Index(fields=["user", "-timestamp"]),
        ]

    def __str__(self):
        return f"{self.status} {self.card_uid} @ {self.timestamp}"


class ApiNonce(models.Model):
    """Replay protection store for X-Nonce values."""

    nonce = models.CharField(max_length=64, unique=True)
    reader = models.ForeignKey(Reader, on_delete=models.CASCADE, related_name="nonces")
    created_at = models.DateTimeField(auto_now_add=True)

    class Meta:
        verbose_name = "Nonce"
        verbose_name_plural = "Nonces"
