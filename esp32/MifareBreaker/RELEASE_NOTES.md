## Release notes

This branch is prepared as a development release for MifareBreaker. Key fixes in this release:
- Fixed unsafe temporary-string usage in darkside attack progress reporting
- Corrected builtin key counting in dictionary attack
- Improved memory handling and ensured owned SD keys are freed
- Polished README and build config

Next steps:
- Hardware testing and timing adjustments
- Optional integration of Proxmark3 crapto1 (GPL) for production-grade Crypto1 support
