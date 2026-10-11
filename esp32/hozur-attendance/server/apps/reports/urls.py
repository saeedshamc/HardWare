from django.urls import path

from . import views

urlpatterns = [
    path("daily/", views.daily_report, name="reports-daily"),
    path("export/csv/", views.export_csv, name="reports-export-csv"),
    path("export/xlsx/", views.export_xlsx, name="reports-export-xlsx"),
]
