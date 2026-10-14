# License note for MifareBreaker

This project currently includes only original code and a development-only Crypto1-like LFSR implementation used for testing and heuristics. It does NOT include or vendor the Proxmark3 crapto1 implementation.

If you (the repository owner) decide to integrate the Proxmark3 `crapto1` implementation or other GPL-licensed source code to improve cryptanalytic accuracy, be aware of the following:

- Proxmark3 code is GPL-licensed. Importing it into this repository will require that the combined work be distributed under the GPL as well.
- You must be comfortable with distributing the entire repository under GPL terms if you merge GPLed code into it.
- Alternatively, keep a clear separation (e.g., optional `crypto/` submodule or a separate repository) containing GPL code to avoid relicensing the main repository.

If you want me to proceed with integrating Proxmark3 code, reply `GPL version` and I will prepare a separate branch with the port and the necessary license notices.
