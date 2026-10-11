from django.contrib import admin

from .models import AttendanceLog


@admin.register(AttendanceLog)
class AttendanceLogAdmin(admin.ModelAdmin):
    list_display = (
        "id",
        "timestamp",
        "user",
        "reader",
        "direction",
        "status",
        "card_uid",
        "card_counter",
    )
    list_filter = ("status", "direction", "reader")
    search_fields = ("card_uid", "user__full_name", "message")
    date_hierarchy = "timestamp"
    readonly_fields = ("created_at",)
