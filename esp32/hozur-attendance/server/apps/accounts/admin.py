from django.contrib import admin

from .models import Person


@admin.register(Person)
class PersonAdmin(admin.ModelAdmin):
    list_display = (
        "id",
        "full_name",
        "role",
        "department_or_class",
        "card_uid",
        "active",
        "blacklisted",
        "last_card_counter",
    )
    list_filter = ("role", "active", "blacklisted", "department_or_class")
    search_fields = ("full_name", "card_uid", "department_or_class")
    readonly_fields = ("created_at", "updated_at")
