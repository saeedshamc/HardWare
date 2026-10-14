# Import helper for Proxmark3 crapto1 (Crypto1)

This branch is intended to hold the vendored Crypto1 implementation (crapto1) imported from the upstream Proxmark3 repository. The import is intentionally isolated to this branch so the repository owner can review and accept license implications before merging to `main`.

This helper does not include the upstream sources itself (to avoid large diffs in the web UI); use the script below to perform the import locally (it will create a single squashed commit). After running the script, push the branch and open a PR to main.

Important: The upstream code is licensed under the GNU General Public License (GPL). By importing and merging this branch into `main` you accept the GPL obligations for the combined work. Do NOT merge until you confirm acceptance.
