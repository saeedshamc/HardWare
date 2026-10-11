from __future__ import annotations

from dataclasses import dataclass
from datetime import datetime, time, timedelta
from typing import Iterable

from django.db.models import QuerySet
from django.utils import timezone
import jdatetime

from apps.accounts.models import Person
from apps.attendance.models import AttendanceLog


@dataclass
class DaySummary:
    person_id: int
    full_name: str
    department: str
    date_jalali: str
    first_in: str | None
    last_out: str | None
    total_hours: float
    late_minutes: int
    absent: bool
    status_note: str


def _local_day_range(day):
    start = timezone.make_aware(datetime.combine(day, time.min))
    end = start + timedelta(days=1)
    return start, end


def _pair_hours(logs: list[AttendanceLog]) -> tuple[float, str | None, str | None]:
    first_in = None
    last_out = None
    total = 0.0
    open_in = None
    for log in logs:
        local = timezone.localtime(log.timestamp)
        t = local.strftime("%H:%M:%S")
        if log.direction == AttendanceLog.Direction.IN:
            if first_in is None:
                first_in = t
            open_in = log.timestamp
        elif log.direction == AttendanceLog.Direction.OUT and open_in is not None:
            last_out = t
            total += (log.timestamp - open_in).total_seconds() / 3600.0
            open_in = None
    return round(total, 2), first_in, last_out


def build_daily_report(
    day=None,
    *,
    work_start_hour: int = 8,
    late_grace_minutes: int = 15,
) -> list[DaySummary]:
    """Daily attendance / lateness / absence / total hours for all active people."""
    if day is None:
        day = timezone.localdate()
    start, end = _local_day_range(day)
    jday = jdatetime.date.fromgregorian(date=day).strftime("%Y/%m/%d")
    late_cutoff = timezone.make_aware(
        datetime.combine(day, time(work_start_hour, 0))
    ) + timedelta(minutes=late_grace_minutes)

    rows: list[DaySummary] = []
    people = Person.objects.filter(active=True).order_by("full_name")
    for person in people:
        logs = list(
            AttendanceLog.objects.filter(
                user=person,
                status=AttendanceLog.Status.OK,
                timestamp__gte=start,
                timestamp__lt=end,
            ).order_by("timestamp")
        )
        hours, first_in, last_out = _pair_hours(logs)
        absent = len(logs) == 0
        late_minutes = 0
        note = "غایب" if absent else "حاضر"
        if not absent and first_in:
            first_log = next(
                (l for l in logs if l.direction == AttendanceLog.Direction.IN), None
            )
            if first_log and first_log.timestamp > late_cutoff:
                late_minutes = int(
                    (first_log.timestamp - late_cutoff).total_seconds() // 60
                )
                note = f"تأخیر {late_minutes} دقیقه"

        rows.append(
            DaySummary(
                person_id=person.pk,
                full_name=person.full_name,
                department=person.department_or_class,
                date_jalali=jday,
                first_in=first_in,
                last_out=last_out,
                total_hours=hours,
                late_minutes=late_minutes,
                absent=absent,
                status_note=note,
            )
        )
    return rows


def summaries_to_csv_rows(rows: Iterable[DaySummary]) -> list[list[str]]:
    header = [
        "user_id",
        "full_name",
        "department",
        "date_jalali",
        "first_in",
        "last_out",
        "total_hours",
        "late_minutes",
        "absent",
        "status",
    ]
    data = [header]
    for r in rows:
        data.append(
            [
                str(r.person_id),
                r.full_name,
                r.department,
                r.date_jalali,
                r.first_in or "",
                r.last_out or "",
                str(r.total_hours),
                str(r.late_minutes),
                "1" if r.absent else "0",
                r.status_note,
            ]
        )
    return data
