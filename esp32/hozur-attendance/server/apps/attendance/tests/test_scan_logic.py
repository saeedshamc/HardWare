from datetime import timedelta

from django.test import TestCase, override_settings
from django.utils import timezone

from apps.accounts.models import Person
from apps.attendance.models import AttendanceLog
from apps.attendance.services import determine_direction, process_offline_sync, process_scan
from apps.readers.models import Reader


class ScanLogicTests(TestCase):
    def setUp(self):
        self.reader = Reader(name="Gate A", location="Main")
        self.reader.set_api_key("test-api-key-1234567890")
        self.reader.default_direction = Reader.DirectionMode.TOGGLE
        self.reader.save()

        self.person = Person.objects.create(
            full_name="Ali Test",
            role=Person.Role.EMPLOYEE,
            department_or_class="IT",
            card_uid="AABBCCDD",
            active=True,
            last_card_counter=1,
        )

    def test_toggle_in_then_out(self):
        t0 = timezone.now()
        r1 = process_scan(
            reader=self.reader,
            uid="AABBCCDD",
            user_id=self.person.pk,
            counter=2,
            when=t0,
        )
        self.assertEqual(r1.status, "ok")
        self.assertEqual(r1.direction, "in")

        r2 = process_scan(
            reader=self.reader,
            uid="AABBCCDD",
            user_id=self.person.pk,
            counter=3,
            when=t0 + timedelta(seconds=30),
        )
        self.assertEqual(r2.status, "ok")
        self.assertEqual(r2.direction, "out")

    def test_dedicated_in_reader(self):
        self.reader.default_direction = Reader.DirectionMode.ALWAYS_IN
        self.reader.save()
        # seed last out so toggle would go in anyway; dedicated forces in twice
        process_scan(
            reader=self.reader,
            uid="AABBCCDD",
            user_id=self.person.pk,
            counter=2,
            when=timezone.now(),
        )
        r = process_scan(
            reader=self.reader,
            uid="AABBCCDD",
            user_id=self.person.pk,
            counter=3,
            when=timezone.now() + timedelta(seconds=30),
        )
        self.assertEqual(r.direction, "in")

    @override_settings(HOZUR_SCAN_COOLDOWN_SEC=60)
    def test_cooldown_duplicate(self):
        t0 = timezone.now()
        process_scan(
            reader=self.reader,
            uid="AABBCCDD",
            user_id=self.person.pk,
            counter=2,
            when=t0,
        )
        r = process_scan(
            reader=self.reader,
            uid="AABBCCDD",
            user_id=self.person.pk,
            counter=3,
            when=t0 + timedelta(seconds=5),
        )
        self.assertEqual(r.status, "duplicate")
        self.assertTrue(
            AttendanceLog.objects.filter(status=AttendanceLog.Status.DUPLICATE).exists()
        )

    def test_counter_not_increasing_denied(self):
        self.person.last_card_counter = 10
        self.person.save()
        r = process_scan(
            reader=self.reader,
            uid="AABBCCDD",
            user_id=self.person.pk,
            counter=10,
            when=timezone.now(),
        )
        self.assertEqual(r.status, "denied")
        self.assertIn("clone", (r.message or "").lower())
        self.assertTrue(
            AttendanceLog.objects.filter(status=AttendanceLog.Status.CLONE_SUSPECT).exists()
        )

    def test_blacklist_denied(self):
        self.person.blacklisted = True
        self.person.save()
        r = process_scan(
            reader=self.reader,
            uid="AABBCCDD",
            user_id=self.person.pk,
            counter=2,
            when=timezone.now(),
        )
        self.assertEqual(r.status, "denied")

    def test_offline_sync_ordered(self):
        scans = [
            {"uid": "AABBCCDD", "user_id": self.person.pk, "counter": 3, "device_time": "t2"},
            {"uid": "AABBCCDD", "user_id": self.person.pk, "counter": 2, "device_time": "t1"},
        ]
        # first by device_time t1 counter 2, then t2 counter 3
        out = process_offline_sync(reader=self.reader, scans=scans)
        self.assertEqual(out["status"], "ok")
        self.assertEqual(out["results"][0]["status"], "ok")
        self.person.refresh_from_db()
        self.assertEqual(self.person.last_card_counter, 3)

    def test_time_window_direction(self):
        self.reader.default_direction = Reader.DirectionMode.TIME_WINDOW
        self.reader.save()
        morning = timezone.now().replace(hour=8, minute=0, second=0, microsecond=0)
        afternoon = timezone.now().replace(hour=15, minute=0, second=0, microsecond=0)
        self.assertEqual(
            determine_direction(self.reader, self.person, morning),
            AttendanceLog.Direction.IN,
        )
        self.assertEqual(
            determine_direction(self.reader, self.person, afternoon),
            AttendanceLog.Direction.OUT,
        )
