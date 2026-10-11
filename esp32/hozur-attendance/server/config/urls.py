from django.conf import settings
from django.conf.urls.static import static
from django.contrib import admin
from django.urls import include, path

admin.site.site_header = "حضور — مدیریت"
admin.site.site_title = "Hozur Admin"
admin.site.index_title = "داشبورد حضور و غیاب"

urlpatterns = [
    path("admin/", admin.site.urls),
    path("admin/reports/", include("apps.reports.urls")),
    path("api/", include("apps.attendance.urls")),
]

if settings.DEBUG:
    urlpatterns += static(settings.MEDIA_URL, document_root=settings.MEDIA_ROOT)
