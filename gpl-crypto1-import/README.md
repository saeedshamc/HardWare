# Quick notes for maintainers

This branch (gpl-crypto1-import) is intentionally minimal and contains helper files and instructions to import Proxmark3's crapto1 implementation. The import is not done automatically by this script — it must be executed locally by a maintainer with git and network access.

Why we do this:
- The upstream project is large and has its own history; using subtree makes review and a single squashed commit easier to audit.
- We keep the import in an isolated branch so the owner can deliberate on license implications.
