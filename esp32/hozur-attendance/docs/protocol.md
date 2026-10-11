# پروتکل Hozur

## چیدمان حافظه کارت (MIFARE Classic 1K)

سکتور پیش‌فرض: **Sector 1**

| بلوک | آدرس مطلق | محتوا |
|------|-----------|--------|
| Data | 4 | `user_id` (4B LE) + `counter` (4B LE) + `crc16` (2B) + pad (6B) |
| — | 5, 6 | رزرو / استفاده نشده |
| Trailer | 7 | KeyA (6) + Access Bits (4) + KeyB (6) |

### مشتق‌سازی کلید (Diversified KeyA)

```
KeyA = HMAC-SHA256(MASTER_KEY, UID || sector_number)[0:6]
KeyB = HMAC-SHA256(MASTER_KEY, UID || sector_number || 0x42)[0:6]
```

- `UID`: بایت‌های خام UID کارت (۴ یا ۷ بایت)
- `sector_number`: یک بایت (مثلاً `0x01`)
- `MASTER_KEY`: فقط روی دستگاه/سرور امن؛ هرگز در ریپو نیست

### Access Bits ایمن (پیش‌فرض enrollment)

هدف: KeyA برای خواندن دیتا و احراز؛ KeyB برای نوشتن/تغییر کلید؛ جلوگیری از دسترسی با کلید پیش‌فرض بعد از enroll.

بایت‌های Access Bits باید قبل از نوشتن با تابع `access_bits_validate()` چک شوند. فلگ `ENROLL_DRY_RUN` از نوشتن واقعی جلوگیری می‌کند.

### CRC16

CRC-CCITT (poly `0x1021`, init `0xFFFF`) روی ۸ بایت اول بلاک دیتا (`user_id` + `counter`).

---

## احراز هویت دستگاه → سرور

همهٔ درخواست‌های API خواننده:

| هدر | معنی |
|-----|------|
| `X-Reader-Id` | شناسه عددی/کد خواننده |
| `X-Api-Key` | کلید خام خواننده (سرور هش ذخیره‌شده را مقایسه می‌کند) |
| `X-Timestamp` | Unix time (ثانیه) |
| `X-Nonce` | رشته یکتا (UUID یا random hex) |
| `X-Signature` | hex از `HMAC-SHA256(api_key, timestamp + "." + nonce + "." + raw_body)` |

Replay: اختلاف `|now - timestamp|` بیش از پنجرهٔ مجاز (مثلاً ۱۲۰ ثانیه) یا nonce تکراری → `401/403`.

---

## `POST /api/scan/`

### Request

```json
{
  "reader_id": 1,
  "uid": "A1B2C3D4",
  "user_id": 42,
  "counter": 17,
  "device_time": "2026-10-11T14:30:00+03:30"
}
```

### Response

```json
{
  "status": "ok",
  "name": "Ali Rezaei",
  "name_latin": "Ali Rezaei",
  "department_or_class": "IT",
  "direction": "in",
  "time": "14:30:00",
  "date_jalali": "1405/07/19",
  "display_image": {
    "format": "rgb565",
    "width": 200,
    "height": 32,
    "data_base64": "..."
  },
  "message": null
}
```

`status`: `ok` | `denied` | `duplicate`

`display_image` اختیاری است؛ اگر نباشد فریم‌ویر `name_latin` را نشان می‌دهد.

---

## `POST /api/enroll/`

```json
{
  "reader_id": 1,
  "uid": "A1B2C3D4",
  "user_id": 42
}
```

پاسخ موفق: کاربر/`card_uid` به‌روز می‌شود.

---

## `GET /api/sync/`

لیست کارت‌های فعال برای کش آفلاین:

```json
{
  "cards": [
    {"uid": "A1B2C3D4", "user_id": 42, "active": true, "last_counter": 17}
  ]
}
```

---

## `POST /api/offline-sync/`

آپلود دسته‌ای صف آفلاین (به ترتیب زمانی):

```json
{
  "reader_id": 1,
  "scans": [
    {
      "uid": "A1B2C3D4",
      "user_id": 42,
      "counter": 18,
      "device_time": "2026-10-11T14:31:00+03:30"
    }
  ]
}
```

---

## رفتار فریم‌ویر بعد از خواندن

1. `PICC_HaltA()` و `PCD_StopCrypto1()`
2. cooldown همان UID برای `CARD_COOLDOWN_MS`
3. در صورت قطع سرور → صف LittleFS + نشانگر Offline روی TFT
