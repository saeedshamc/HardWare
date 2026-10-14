# Third-party components

This repository depends on or optionally integrates the following third-party projects.

- Proxmark3 (optional, Crypto1 implementation)
  - Purpose: production-grade Crypto1 keystream / cryptanalysis routines (crapto1)
  - Upstream: https://github.com/Proxmark/proxmark3
  - Integration options:
    - Submodule: the preferred, low-risk option. Keep upstream code as a submodule at esp32/MifareBreaker/crypto/crapto1.
    - Vendor: copy upstream files into a dedicated branch `gpl-crypto1-import`. Merging that branch into `main` requires accepting GPL licence implications.

See esp32/MifareBreaker/LICENSE_NOTE.md for more details.
