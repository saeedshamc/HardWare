from __future__ import annotations

from mifare_keylab.config import AppConfig
from mifare_keylab.readers.interface.base import ReaderInterface


def create_reader(cfg: AppConfig) -> ReaderInterface:
    name = (cfg.reader or "simulator").lower()
    if name == "rc522":
        from mifare_keylab.readers.rc522.serial_reader import Rc522SerialReader

        if not cfg.serial_port:
            raise SystemExit("RC522 reader requires --port / config.serial_port")
        return Rc522SerialReader(cfg.serial_port, cfg.baud)
    if name in {"chameleon", "pn532", "proxmark3"}:
        raise SystemExit(
            f"Reader backend '{name}' is stubbed — use 'simulator' or 'rc522' in v0.5"
        )
    from mifare_keylab.readers.simulator.sim_reader import SimulatorReader

    return SimulatorReader()
