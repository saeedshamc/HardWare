import pytest

from mifare_keylab.candidates.generator import known_prefix, structured_range, visualize_space
from mifare_keylab.candidates.validator import load_dictionary, normalize_key


def test_normalize_key():
    assert normalize_key("a0a1a2a3a4a5") == "A0A1A2A3A4A5"
    assert normalize_key("bad") is None


def test_structured_bounded():
    keys = structured_range("000000000000", "000000000002", limit=10)
    assert keys == ["000000000000", "000000000001", "000000000002"]


def test_structured_refuses_huge():
    with pytest.raises(ValueError):
        structured_range("000000000000", "FFFFFFFFFFFF", limit=100)


def test_known_prefix_small():
    keys = known_prefix("A0A1A2A3A4", limit=300)  # 2 nibbles -> 256
    assert len(keys) == 256
    assert keys[0].startswith("A0A1A2A3A4")


def test_visualize_contains_full_space():
    text = visualize_space("A0A1A2")
    assert "2^48" in text
    assert "24 bits" in text


def test_load_dictionary(tmp_path):
    p = tmp_path / "keys.txt"
    p.write_text("FFFFFFFFFFFF\n# comment\nbad\n112233445566\n", encoding="utf-8")
    assert load_dictionary(str(p)) == ["FFFFFFFFFFFF", "112233445566"]
