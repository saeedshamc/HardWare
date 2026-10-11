from __future__ import annotations

import base64
import logging
import struct
from pathlib import Path

from django.conf import settings

logger = logging.getLogger("hozour")


def _reshape_persian(text: str) -> str:
    try:
        import arabic_reshaper
        from bidi.algorithm import get_display

        reshaped = arabic_reshaper.reshape(text)
        return get_display(reshaped)
    except Exception:
        return text


def render_name_bitmap(name: str, width: int = 200, height: int = 32) -> dict | None:
    """
    Render Persian/Latin name to RGB565 little-endian bitmap for ESP32 TFT.
    Returns None if Pillow/font unavailable (firmware falls back to Latin string).
    """
    if not name:
        return None
    try:
        from PIL import Image, ImageDraw, ImageFont
    except Exception:
        return None

    font_path = Path(settings.FONT_PATH)
    try:
        if font_path.is_file():
            font = ImageFont.truetype(str(font_path), 22)
        else:
            font = ImageFont.load_default()
            logger.warning("Vazirmatn font missing at %s — using default", font_path)
    except Exception:
        font = ImageFont.load_default()

    display_text = _reshape_persian(name)
    img = Image.new("RGB", (width, height), (0, 40, 0))
    draw = ImageDraw.Draw(img)
    bbox = draw.textbbox((0, 0), display_text, font=font)
    tw, th = bbox[2] - bbox[0], bbox[3] - bbox[1]
    x = max(0, (width - tw) // 2)
    y = max(0, (height - th) // 2)
    draw.text((x, y), display_text, fill=(255, 255, 255), font=font)

    pixels = []
    for py in range(height):
        for px in range(width):
            r, g, b = img.getpixel((px, py))
            rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
            pixels.append(struct.pack("<H", rgb565))
    raw = b"".join(pixels)
    return {
        "format": "rgb565",
        "width": width,
        "height": height,
        "data_base64": base64.b64encode(raw).decode("ascii"),
    }
