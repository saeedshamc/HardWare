from django.contrib import admin, messages

from .models import Reader


@admin.register(Reader)
class ReaderAdmin(admin.ModelAdmin):
    list_display = ("id", "name", "location", "default_direction", "api_key_prefix", "active")
    list_filter = ("active", "default_direction")
    search_fields = ("name", "location", "api_key_prefix")
    readonly_fields = ("api_key_hash", "api_key_prefix", "created_at")

    def save_model(self, request, obj, form, change):
        if not change or not obj.api_key_hash:
            raw = obj.set_api_key()
            super().save_model(request, obj, form, change)
            messages.warning(
                request,
                f"API Key خواننده (فقط یک‌بار نمایش): {raw}",
            )
        else:
            super().save_model(request, obj, form, change)

    actions = ["rotate_api_key"]

    @admin.action(description="چرخش API Key")
    def rotate_api_key(self, request, queryset):
        for reader in queryset:
            raw = reader.set_api_key()
            reader.save(update_fields=["api_key_hash", "api_key_prefix"])
            messages.warning(request, f"{reader}: {raw}")
