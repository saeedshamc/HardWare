from mifare_keylab.authentication.tester import run_candidates, run_single
from mifare_keylab.config import AppConfig
from mifare_keylab.readers.simulator.sim_reader import SimulatorReader


def test_sim_auth_success():
    r = SimulatorReader()
    r.open()
    r.detect_card()
    res = run_single(r, 1, "A", "112233445566")
    assert res.ok
    r.close()


def test_sim_dictionary_finds_key():
    r = SimulatorReader()
    r.open()
    r.detect_card()
    cfg = AppConfig(privacy=True)
    stats = run_candidates(
        r,
        1,
        "A",
        ["FFFFFFFFFFFF", "000000000000", "112233445566"],
        cfg,
        console=None,
    )
    assert stats.matched_key == "112233445566"
    assert stats.successes == 1
    assert stats.failures == 2
    r.close()
