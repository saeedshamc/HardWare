# License note for MifareBreaker (updated)

This project may optionally use the Crypto1 implementation from the Proxmark3 upstream repository. Because that code is distributed under the GPL, including it directly in this repository (vendoring / copying upstream files into the tree) has legal/licensing implications.

Two integration options are supported and provided here:

1) Submodule (recommended, low-risk for main)
   - We add a git submodule at esp32/MifareBreaker/crypto/crapto1 that points to the upstream Proxmark3 repo.
   - This keeps upstream code clearly separate and attributed. The main repository itself does not need to be relicensed to GPL simply to reference the submodule, but redistribution of a combined binary that includes the upstream code must respect the upstream GPL license.
   - To fetch the submodule after cloning this repo:
     git submodule update --init --recursive esp32/MifareBreaker/crypto/crapto1

   - Upstream source (recommended): https://github.com/Proxmark/proxmark3

2) Vendoring (explicit, requires owner approval)
   - This copies the upstream GPLed source code directly into esp32/MifareBreaker/crypto/crapto1.
   - If you choose to vendor GPL code into this repository and then merge that branch into `main`, the combined work will typically need to be distributed under GPL according to upstream terms. Do NOT vendor GPL code into `main` unless you explicitly accept relicensing the repository under the same GPL terms.
   - For vendoring we provide a dedicated branch `gpl-crypto1-import` containing helper scripts and an import manifest. Keep vendored GPL code isolated to that branch unless you confirm "vendor-to-main" and accept the license change.

Attribution and provenance
- When adding upstream code (either via submodule or vendoring), include the following metadata in esp32/MifareBreaker/crypto/crapto1/README_UPSTREAM.md:
  - upstream URL
  - upstream commit SHA or tag
  - upstream license identifier (GPLv2/GPLv3/other)
  - list of upstream authors if available

For questions about licensing or if you want me to vendor the code into the repo (and change LICENSE accordingly), reply with: "vendor-gpl-approve" and I will prepare the vendor branch and PR with explicit license text and notices.

