import hashlib
import secrets

from django.db import models


class Reader(models.Model):
    class DirectionMode(models.TextChoices):
        TOGGLE = "toggle", "سوییچ خودکار"
        ALWAYS_IN = "in", "همیشه ورود"
        ALWAYS_OUT = "out", "همیشه خروج"
        TIME_WINDOW = "time_window", "پنجره زمانی"

    name = models.CharField("نام", max_length=120)
    location = models.CharField("محل", max_length=200, blank=True)
    api_key_hash = models.CharField("هش API Key", max_length=64, editable=False)
    api_key_prefix = models.CharField("پیشوند کلید", max_length=8, editable=False, blank=True)
    default_direction = models.CharField(
        "حالت جهت",
        max_length=20,
        choices=DirectionMode.choices,
        default=DirectionMode.TOGGLE,
    )
    active = models.BooleanField("فعال", default=True)
    created_at = models.DateTimeField(auto_now_add=True)

    class Meta:
        verbose_name = "خواننده"
        verbose_name_plural = "خواننده‌ها"

    def __str__(self):
        return f"{self.name} (#{self.pk})"

    @staticmethod
    def hash_key(raw_key: str) -> str:
        return hashlib.sha256(raw_key.encode("utf-8")).hexdigest()

    def set_api_key(self, raw_key: str | None = None) -> str:
        raw = raw_key or secrets.token_urlsafe(24)
        self.api_key_hash = self.hash_key(raw)
        self.api_key_prefix = raw[:8]
        return raw

    def check_api_key(self, raw_key: str) -> bool:
        if not raw_key or not self.api_key_hash:
            return False
        return secrets.compare_digest(self.api_key_hash, self.hash_key(raw_key))
