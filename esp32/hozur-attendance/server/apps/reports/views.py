import csv
import io
from datetime import datetime

from django.contrib.admin.views.decorators import staff_member_required
from django.http import HttpResponse
from django.shortcuts import render
from django.utils import timezone
from django.utils.encoding import smart_str

from .services import build_daily_report, summaries_to_csv_rows


def _parse_day(request):
    raw = request.GET.get("day")
    if not raw:
        return timezone.localdate()
    try:
        return datetime.strptime(raw, "%Y-%m-%d").date()
    except ValueError:
        return timezone.localdate()


@staff_member_required
def daily_report(request):
    day = _parse_day(request)
    rows = build_daily_report(day)
    return render(
        request,
        "admin/reports/daily.html",
        {
            "title": "گزارش روزانه حضور",
            "day": day,
            "rows": rows,
            "site_header": "حضور — Hozur",
        },
    )


@staff_member_required
def export_csv(request):
    day = _parse_day(request)
    rows = summaries_to_csv_rows(build_daily_report(day))
    resp = HttpResponse(content_type="text/csv; charset=utf-8")
    resp["Content-Disposition"] = f'attachment; filename="hozour_{day}.csv"'
    resp.write("\ufeff")  # Excel-friendly UTF-8 BOM
    writer = csv.writer(resp)
    for row in rows:
        writer.writerow([smart_str(c) for c in row])
    return resp


@staff_member_required
def export_xlsx(request):
    from openpyxl import Workbook

    day = _parse_day(request)
    rows = summaries_to_csv_rows(build_daily_report(day))
    wb = Workbook()
    ws = wb.active
    ws.title = "daily"
    for r in rows:
        ws.append(r)
    buf = io.BytesIO()
    wb.save(buf)
    buf.seek(0)
    resp = HttpResponse(
        buf.getvalue(),
        content_type="application/vnd.openxmlformats-officedocument.spreadsheetml.sheet",
    )
    resp["Content-Disposition"] = f'attachment; filename="hozour_{day}.xlsx"'
    return resp
