from datetime import timedelta

from django.test import TestCase
from django.utils import timezone

from apps.accounts.models import Person
from apps.attendance.models import AttendanceLog
from apps.attendance.services import process_scan
from apps.readers.models import Reader
from apps.reports.services import build_daily_report


class DailyReportTests(TestCase):
    def setUp(self):
        self.reader = Reader(name="R1")
        self.reader.set_api_key("k" * 24)
        self.reader.save()
        self.person = Person.objects.create(
            full_name="Sara",
            card_uid="11223344",
            active=True,
            last_card_counter=0,
        )

    def test_hours_and_presence(self):
        t0 = timezone.now().replace(hour=8, minute=0, second=0, microsecond=0)
        process_scan(
            reader=self.reader,
            uid="11223344",
            user_id=self.person.pk,
            counter=1,
            when=t0,
            skip_cooldown=True,
        )
        process_scan(
            reader=self.reader,
            uid="11223344",
            user_id=self.person.pk,
            counter=2,
            when=t0 + timedelta(hours=8),
            skip_cooldown=True,
        )
        rows = build_daily_report(timezone.localdate())
        mine = next(r for r in rows if r.person_id == self.person.pk)
        self.assertFalse(mine.absent)
        self.assertGreaterEqual(mine.total_hours, 7.9)
        self.assertEqual(
            AttendanceLog.objects.filter(status=AttendanceLog.Status.OK).count(), 2
        )
