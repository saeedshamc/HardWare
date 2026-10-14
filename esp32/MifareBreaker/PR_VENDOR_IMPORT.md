# PR Description template for vendor import (gpl-crypto1-import → main)

Title: Import crapto1 (Crypto1) from Proxmark3 (GPL) — gpl-crypto1-import → main

Body:

Summary:
This PR imports the crapto1 Crypto1 implementation from Proxmark3 into esp32/MifareBreaker/crypto/crapto1 to provide a production-grade Crypto1 keystream implementation used by nested/darkside attacks.

Upstream:
- Repository: https://github.com/Proxmark/proxmark3
- Suggested branch/tag: master (or specify exact tag/commit)

License & legal notes:
- Upstream code is licensed under the GNU General Public License (GPL). The upstream COPYING file is included in the imported folder.
- Merging this PR into `main` will likely require the combined work to be distributed under the GPL. The repository owner must explicitly acknowledge and accept this relicensing before merging.

Checklist:
- [ ] Owner confirms acceptance of relicensing obligations for main (if merging)
- [ ] CI: PlatformIO build passes
- [ ] Hardware smoke test (flash + known test card); provide serial logs

Notes:
- If you prefer not to change licensing of `main`, keep this branch separate and use it only for GPL-distributed builds.
