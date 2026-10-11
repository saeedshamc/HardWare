from django.urls import path

from .views import DeriveKeyView, EnrollView, OfflineSyncView, ScanView, SyncView

urlpatterns = [
    path("scan/", ScanView.as_view(), name="api-scan"),
    path("enroll/", EnrollView.as_view(), name="api-enroll"),
    path("sync/", SyncView.as_view(), name="api-sync"),
    path("offline-sync/", OfflineSyncView.as_view(), name="api-offline-sync"),
    path("derive-key/", DeriveKeyView.as_view(), name="api-derive-key"),
]
