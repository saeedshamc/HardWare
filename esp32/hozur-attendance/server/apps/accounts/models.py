from django.db import models


class Person(models.Model):
    class Role(models.TextChoices):
        EMPLOYEE = "employee", "کارمند"
        STUDENT = "student", "دانشجو"

    full_name = models.CharField("نام کامل", max_length=200)
    role = models.CharField(
        "نقش", max_length=20, choices=Role.choices, default=Role.EMPLOYEE
    )
    department_or_class = models.CharField("دپارتمان / کلاس", max_length=120, blank=True)
    card_uid = models.CharField(
        "UID کارت", max_length=32, unique=True, null=True, blank=True, db_index=True
    )
    active = models.BooleanField("فعال", default=True)
    blacklisted = models.BooleanField("لیست سیاه / مفقودی", default=False)
    photo = models.ImageField("عکس", upload_to="photos/", blank=True, null=True)
    last_card_counter = models.PositiveIntegerField("آخرین شمارنده کارت", default=0)
    created_at = models.DateTimeField(auto_now_add=True)
    updated_at = models.DateTimeField(auto_now=True)

    class Meta:
        verbose_name = "کاربر"
        verbose_name_plural = "کاربران"
        ordering = ["full_name"]

    def __str__(self):
        return f"{self.full_name} ({self.pk})"
