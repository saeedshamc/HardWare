from __future__ import annotations

import logging
from dataclasses import dataclass
from datetime import timedelta
from typing import Any

from django.conf import settings
from django.db import transaction
from django.utils import timezone
import jdatetime

from apps.accounts.models import Person
from apps.readers.models import Reader

from .models import AttendanceLog
from .name_bitmap import render_name_bitmap

logger = logging.getLogger("hozour")


@dataclass
class ScanResult:
    status: str
    name: str = ""
    name_latin: str = ""
    department_or_class: str = ""
    direction: str = ""
    time: str = ""
    date_jalali: str = ""
    display_image: dict[str, Any] | None = None
    message: str | None = None


def determine_direction(reader: Reader, user: Person | None, when) -> str:
    mode = reader.default_direction
    if mode == Reader.DirectionMode.ALWAYS_IN:
        return AttendanceLog.Direction.IN
    if mode == Reader.DirectionMode.ALWAYS_OUT:
        return AttendanceLog.Direction.OUT
    if mode == Reader.DirectionMode.TIME_WINDOW:
        local = timezone.localtime(when)
        if local.hour < settings.HOZUR_IN_BEFORE_HOUR:
            return AttendanceLog.Direction.IN
        return AttendanceLog.Direction.OUT

    # toggle
    if user is None:
        return AttendanceLog.Direction.IN
    last = (
        AttendanceLog.objects.filter(user=user, status=AttendanceLog.Status.OK)
        .order_by("-timestamp")
        .first()
    )
    if last is None or last.direction == AttendanceLog.Direction.OUT:
        return AttendanceLog.Direction.IN
    return AttendanceLog.Direction.OUT


def _jalali_parts(when):
    local = timezone.localtime(when)
    j = jdatetime.datetime.fromgregorian(datetime=local)
    return j.strftime("%H:%M:%S"), j.strftime("%Y/%m/%d")


@transaction.atomic
def process_scan(
    *,
    reader: Reader,
    uid: str,
    user_id: int | None,
    counter: int,
    device_time: str = "",
    when=None,
    skip_cooldown: bool = False,
) -> ScanResult:
    when = when or timezone.now()
    uid = (uid or "").strip().upper()
    time_str, date_jalali = _jalali_parts(when)

    person = None
    if user_id:
        person = Person.objects.filter(pk=user_id).first()
    if person is None and uid:
        person = Person.objects.filter(card_uid__iexact=uid).first()

    def deny(message: str, status=AttendanceLog.Status.DENIED, user=None) -> ScanResult:
        AttendanceLog.objects.create(
            user=user,
            reader=reader,
            direction="",
            timestamp=when,
            card_uid=uid,
            card_counter=counter,
            status=status,
            message=message,
            device_time=device_time or "",
        )
        logger.info("scan denied uid=%s reason=%s", uid, message)
        return ScanResult(
            status="denied",
            message=message,
            time=time_str,
            date_jalali=date_jalali,
            name_latin=user.full_name if user else "",
            name=user.full_name if user else "",
        )

    if person is None:
        return deny("unknown card/user")

    if not person.active:
        return deny("inactive user", user=person)

    if person.blacklisted:
        return deny("blacklisted card", user=person)

    if person.card_uid and person.card_uid.upper() != uid:
        return deny("uid mismatch", user=person)

    # Counter anti-clone: must be strictly greater than last known
    if counter <= person.last_card_counter:
        AttendanceLog.objects.create(
            user=person,
            reader=reader,
            direction="",
            timestamp=when,
            card_uid=uid,
            card_counter=counter,
            status=AttendanceLog.Status.CLONE_SUSPECT,
            message="counter not increasing",
            device_time=device_time or "",
        )
        logger.warning(
            "clone suspect uid=%s counter=%s last=%s",
            uid,
            counter,
            person.last_card_counter,
        )
        return ScanResult(
            status="denied",
            name=person.full_name,
            name_latin=person.full_name,
            message="possible cloned card",
            time=time_str,
            date_jalali=date_jalali,
        )

    # Cooldown / anti-passback window (skipped for ordered offline bulk sync)
    recent = None
    if not skip_cooldown:
        cooldown = timedelta(seconds=settings.HOZUR_SCAN_COOLDOWN_SEC)
        recent = (
            AttendanceLog.objects.filter(
                user=person,
                status=AttendanceLog.Status.OK,
                timestamp__gte=when - cooldown,
            )
            .order_by("-timestamp")
            .first()
        )
    if recent:
        AttendanceLog.objects.create(
            user=person,
            reader=reader,
            direction=recent.direction,
            timestamp=when,
            card_uid=uid,
            card_counter=counter,
            status=AttendanceLog.Status.DUPLICATE,
            message="cooldown",
            device_time=device_time or "",
        )
        img = render_name_bitmap(person.full_name)
        return ScanResult(
            status="duplicate",
            name=person.full_name,
            name_latin=person.full_name,
            department_or_class=person.department_or_class,
            direction=recent.direction,
            time=time_str,
            date_jalali=date_jalali,
            display_image=img,
            message="duplicate within cooldown",
        )

    direction = determine_direction(reader, person, when)
    person.last_card_counter = counter
    if not person.card_uid:
        person.card_uid = uid
    person.save(update_fields=["last_card_counter", "card_uid", "updated_at"])

    AttendanceLog.objects.create(
        user=person,
        reader=reader,
        direction=direction,
        timestamp=when,
        card_uid=uid,
        card_counter=counter,
        status=AttendanceLog.Status.OK,
        device_time=device_time or "",
    )

    img = render_name_bitmap(person.full_name)
    return ScanResult(
        status="ok",
        name=person.full_name,
        name_latin=person.full_name,
        department_or_class=person.department_or_class,
        direction=direction,
        time=time_str,
        date_jalali=date_jalali,
        display_image=img,
    )


def process_enroll(*, reader: Reader, uid: str, user_id: int) -> dict:
    uid = (uid or "").strip().upper()
    person = Person.objects.filter(pk=user_id).first()
    if person is None:
        return {"status": "denied", "message": "user not found"}
    if Person.objects.filter(card_uid__iexact=uid).exclude(pk=person.pk).exists():
        return {"status": "denied", "message": "uid already assigned"}
    person.card_uid = uid
    person.last_card_counter = 1
    person.save(update_fields=["card_uid", "last_card_counter", "updated_at"])
    return {
        "status": "ok",
        "user_id": person.pk,
        "uid": uid,
        "name": person.full_name,
    }


def process_offline_sync(*, reader: Reader, scans: list[dict]) -> dict:
    results = []
    base = timezone.now()
    ordered = sorted(scans, key=lambda s: s.get("device_time") or "")
    for idx, item in enumerate(ordered):
        res = process_scan(
            reader=reader,
            uid=item.get("uid", ""),
            user_id=item.get("user_id"),
            counter=int(item.get("counter") or 0),
            device_time=item.get("device_time") or "",
            when=base + timedelta(seconds=idx),
            skip_cooldown=True,
        )
        results.append(
            {
                "uid": item.get("uid"),
                "status": res.status,
                "message": res.message,
            }
        )
    return {"status": "ok", "results": results}


def scan_result_to_dict(res: ScanResult) -> dict:
    payload = {
        "status": res.status,
        "name": res.name,
        "name_latin": res.name_latin,
        "department_or_class": res.department_or_class,
        "direction": res.direction,
        "time": res.time,
        "date_jalali": res.date_jalali,
        "message": res.message,
    }
    if res.display_image:
        payload["display_image"] = res.display_image
    return payload
