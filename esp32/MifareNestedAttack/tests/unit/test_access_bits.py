import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
TOOL = ROOT / "tools" / "access_bits" / "decode_access_bits.py"


def test_encoded_zero_conditions_valid():
    out = subprocess.check_output(
        [sys.executable, str(TOOL), "FF", "0F", "00"], text=True
    )
    assert "Valid: True" in out


def test_garbage_invalid():
    out = subprocess.check_output(
        [sys.executable, str(TOOL), "AA", "BB", "CC"], text=True
    )
    assert "INVALID: False" in out
