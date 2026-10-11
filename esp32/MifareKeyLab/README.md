# MIFARE KeyLab

> An educational MIFARE Classic security laboratory for authorized key testing, authentication analysis, candidate-key testing, and MIFARE Classic security research.

---

## Overview

**MIFARE KeyLab** is an educational RFID security tool designed for working with **MIFARE Classic** test cards.

The project provides a controlled environment for studying:

* MIFARE Classic memory organization
* Sector authentication
* Key A / Key B
* Access conditions
* Crypto1 authentication
* Candidate-key testing
* Dictionary-based key testing
* Authentication behavior
* Key-space analysis
* Nested Attack concepts
* Defensive analysis

The primary hardware target is:

```text
Chameleon Ultra
      +
MIFARE Classic
```

with optional support for:

```text
ESP32 + RC522
ESP32 + PN532
Proxmark3
```

---

# ⚠️ Authorized Laboratory Use

This project is intended for:

* Cards owned by the researcher
* Dedicated laboratory cards
* Test cards
* Classroom demonstrations
* Authorized security research

Do not use the software against cards, access-control systems, transportation cards, payment systems, or other credentials without explicit authorization.

---

# 🎯 Project Goals

The project has four major goals:

```text
┌────────────────────────────────────────┐
│             MIFARE KeyLab              │
├────────────────────────────────────────┤
│                                        │
│  1. Card Analysis                      │
│  2. Authentication Testing             │
│  3. Candidate-Key Research             │
│  4. MIFARE Security Education          │
│                                        │
└────────────────────────────────────────┘
```

The project is **not** intended to be a generic access-card cracking tool.

The primary objective is to provide a reproducible security laboratory.

---

# 🧠 MIFARE Classic Basics

MIFARE Classic uses sector-based memory organization.

For a typical MIFARE Classic 1K:

```text
1 KB EEPROM
│
├── Sector 0
│   ├── Block 0
│   ├── Block 1
│   ├── Block 2
│   └── Block 3
│
├── Sector 1
│   ├── Block 4
│   ├── Block 5
│   ├── Block 6
│   └── Block 7
│
├── ...
│
└── Sector 15
    ├── Block 60
    ├── Block 61
    ├── Block 62
    └── Block 63
```

Each sector contains:

```text
Data blocks
     +
Sector Trailer
```

The sector trailer contains:

```text
┌────────────┬──────────────┬────────────┐
│   Key A    │ Access Bits  │   Key B    │
│  6 bytes   │   4 bytes    │  6 bytes   │
└────────────┴──────────────┴────────────┘
```

---

# 🔐 Key Space

MIFARE Classic keys are 6 bytes:

```text
6 bytes × 8 bits
=
48 bits
```

Therefore:

```text
2^48
=
281,474,976,710,656
```

possible keys exist.

This means:

```text
Full 48-bit brute force
        ↓
~281 trillion candidates
        ↓
Not practical for a normal classroom setup
```

Therefore KeyLab focuses on:

```text
Candidate Lists
      │
      ├── Known laboratory keys
      ├── Dictionary keys
      ├── User supplied candidates
      ├── Structured key patterns
      └── Research-generated candidates
```

rather than blindly enumerating the entire 48-bit space.

---

# 🧪 Example Candidate File

Create:

```text
keys.txt
```

Example:

```text
FFFFFFFFFFFF
A0A1A2A3A4A5
D3F7D3F7D3F7
000000000000
AABBCCDDEEFF
112233445566
```

Each line represents one 6-byte MIFARE Classic key.

Format:

```text
12 hexadecimal characters
```

Example:

```text
A0A1A2A3A4A5
```

---

# 🚀 Basic Workflow

```text
              MIFARE Classic
                    │
                    ▼
              Card Detection
                    │
                    ▼
             Select Card
                    │
                    ▼
             Select Sector
                    │
                    ▼
          ┌─────────┴─────────┐
          │                   │
       Key A                Key B
          │                   │
          └─────────┬─────────┘
                    ▼
             Candidate List
                    │
                    ▼
             Authentication
                    │
          ┌─────────┴─────────┐
          │                   │
       Failure              Success
          │                   │
          ▼                   ▼
       Next Key           Save Result
```

---

# 🖥️ CLI

The primary interface is command-line based.

Example:

```bash
mifare-keylab info
```

Output:

```text
MIFARE KeyLab
=============

Reader       : Chameleon Ultra
Card         : MIFARE Classic 1K
UID          : XX XX XX XX
ATQA         : XX XX
SAK          : XX
```

---

# 🔍 Card Information

```bash
mifare-keylab card info
```

Example:

```text
Card Information
----------------

UID       : 04 XX XX XX
ATQA      : 00 04
SAK       : 08

Card Type : MIFARE Classic 1K
Sectors   : 16
Blocks    : 64
```

---

# 🔐 Authentication Test

Test a specific key against an authorized laboratory card:

```bash
mifare-keylab auth \
    --sector 1 \
    --type A \
    --key A0A1A2A3A4A5
```

Example:

```text
[*] Sector : 1
[*] Key    : A
[*] Testing candidate...

[+] Authentication successful

Sector:
    1

Key type:
    A

Status:
    VALID
```

---

# 📖 Candidate-Key Testing

Use a laboratory candidate file:

```bash
mifare-keylab test \
    --sector 1 \
    --type A \
    --keys ./keys.txt
```

Output:

```text
========================================
 MIFARE KEYLAB
 Candidate Key Tester
========================================

Card:
    MIFARE Classic 1K

Sector:
    1

Key:
    A

Candidates:
    6

----------------------------------------

[01/06] FFFFFFFFFFFF   FAIL
[02/06] A0A1A2A3A4A5   FAIL
[03/06] D3F7D3F7D3F7   FAIL
[04/06] 000000000000   FAIL
[05/06] AABBCCDDEEFF   FAIL
[06/06] 112233445566   SUCCESS

----------------------------------------

[+] Candidate matched

Key:
    112233445566
```

---

# 📊 Progress Display

For larger candidate sets:

```text
Testing candidates...

[████████████████░░░░] 78%

Tested:
    7,821

Remaining:
    2,179

Rate:
    XXXX keys/sec

Elapsed:
    00:03:17
```

The tool should not assume that the reported rate is constant; RF communication, reader firmware, retries, and card behavior can affect throughput.

---

# 🧮 Search Modes

KeyLab supports several controlled search modes.

## 1. Dictionary

```bash
mifare-keylab test \
    --keys keys.txt
```

---

## 2. Structured Candidates

Example conceptual patterns:

```text
000000000000
000000000001
000000000002
...
```

or:

```text
112233445566
112233445567
112233445568
```

This mode should be explicitly bounded:

```bash
--start
--end
```

rather than allowing an accidental unbounded search.

---

## 3. Known Prefix

If laboratory information tells us:

```text
A0A1A2??????
```

the candidate generator can create only the remaining candidate space.

Example:

```text
A0A1A2000000
A0A1A2000001
A0A1A2000002
...
```

This can be useful for teaching how additional information dramatically reduces a search space.

---

# 🧠 Search-Space Visualization

KeyLab should visualize the search space.

Example:

```text
Full key space

2^48
│
├───────────────────────────────────────────
│
│             281,474,976,710,656
│
└───────────────────────────────────────────


Known prefix:

A0 A1 A2 ?? ?? ??

Remaining:
24 bits

2^24
│
├──────────────────────
│
│ 16,777,216 candidates
│
└──────────────────────
```

This gives students a practical understanding of entropy.

---

# 🔬 Nested Attack Module

Nested Attack is a separate module from ordinary candidate testing.

Conceptually:

```text
Known Key
    │
    ▼
Authentication
    │
    ▼
Crypto1 Session
    │
    ▼
Nested Authentication
    │
    ▼
Cryptographic Analysis
    │
    ▼
Candidate Key Recovery
```

The architecture keeps this functionality separated:

```text
keylab/
│
├── readers/
│
├── mifare/
│
├── authentication/
│
├── candidates/
│
├── analysis/
│
└── nested/
```

This makes it possible to test each component independently.

---

# 🧬 Crypto1 Research Module

The Crypto1 module provides educational analysis of:

* authentication state
* nonce relationships
* pseudo-random number generation
* keystream concepts
* encrypted communication
* authentication transitions

Example:

```bash
mifare-keylab crypto demo
```

Output:

```text
Crypto1 Demonstration
=====================

Authentication State
--------------------

Reader nonce : ********
Card nonce   : ********

Session:
    ESTABLISHED

Cryptographic State:
    [visualized]

Keystream:
    [educational representation]
```

Sensitive values should be hidden by default.

---

# 🧪 Laboratory Mode

KeyLab has a dedicated laboratory mode:

```bash
mifare-keylab lab
```

The interface becomes:

```text
=========================================
          MIFARE SECURITY LAB
=========================================

Card
----
[1] Detect
[2] Information
[3] Sector Map

Authentication
--------------
[4] Test Key
[5] Candidate List

Analysis
--------
[6] Access Conditions
[7] Crypto1 Demo
[8] Nested Research

Utilities
---------
[9] Export Report
[0] Exit
```

---

# 📝 Laboratory Reports

The tool can generate reports.

Example:

```bash
mifare-keylab report \
    --output report.json
```

Example:

```json
{
  "card_type": "MIFARE Classic 1K",
  "uid": "REDACTED",
  "tested_sectors": [1, 2, 3],
  "authentication_tests": 120,
  "matches": 2,
  "timestamp": "..."
}
```

UID and recovered keys should be redacted in reports by default.

---

# 🔒 Privacy Mode

Enable:

```bash
mifare-keylab config privacy --enable
```

This changes output to:

```text
UID:
    [REDACTED]

Key:
    [REDACTED]

Authentication:
    SUCCESS
```

This is useful when screenshots are used in teaching material.

---

# 🏗️ Architecture

```text
mifare-keylab/
│
├── cli/
│   ├── commands/
│   └── output/
│
├── readers/
│   ├── interface/
│   ├── chameleon/
│   ├── rc522/
│   └── pn532/
│
├── mifare/
│   ├── card/
│   ├── sector/
│   ├── block/
│   ├── trailer/
│   └── access_bits/
│
├── authentication/
│   ├── auth_a/
│   ├── auth_b/
│   └── session/
│
├── candidates/
│   ├── dictionary/
│   ├── generator/
│   └── validator/
│
├── crypto/
│   ├── crypto1/
│   ├── prng/
│   └── visualization/
│
├── nested/
│   ├── simulator/
│   └── analysis/
│
├── reports/
│
├── tests/
│
├── docs/
│
└── README.md
```

---

# 🔌 Reader Abstraction

The application should use a common reader interface.

```text
ReaderInterface
       │
       ├── RC522
       │
       ├── PN532
       │
       ├── Chameleon Ultra
       │
       └── Proxmark3
```

Conceptual API:

```text
detect_card()
get_card_info()
authenticate()
read_block()
write_block()
stop_crypto1()
```

This allows the same CLI to work with different hardware backends.

---

# 🧰 Recommended Hardware

## Development

```text
ESP32
+
RC522
```

Best for:

* learning SPI
* RFID basics
* authentication
* memory analysis

---

## NFC Development

```text
ESP32
+
PN532
```

Best for:

* NFC development
* embedded projects
* multiple NFC protocols

---

## MIFARE Security Research

```text
Chameleon Ultra
```

Best suited to:

* MIFARE Classic research
* card emulation
* controlled security experiments
* educational demonstrations

---

## Advanced RFID Research

```text
Proxmark3
```

Best suited to:

* low-level RFID research
* protocol analysis
* advanced experimentation
* broad RFID/NFC research

---

# 📚 Classroom Example

A complete lesson can follow this sequence:

```text
Lesson 1
   │
   ├── RFID fundamentals
   │
   ▼
Lesson 2
   │
   ├── MIFARE memory
   │
   ▼
Lesson 3
   │
   ├── Authentication
   │
   ▼
Lesson 4
   │
   ├── Key space
   │
   ▼
Lesson 5
   │
   ├── Candidate-key testing
   │
   ▼
Lesson 6
   │
   ├── Crypto1
   │
   ▼
Lesson 7
   │
   ├── Nested Attack concept
   │
   ▼
Lesson 8
   │
   └── Defensive design
```

---

# 🧑‍🏫 Example Classroom Experiment

The instructor prepares a dedicated MIFARE Classic test card.

Known information:

```text
Card:
    MIFARE Classic 1K

Sector:
    4

Key A:
    known

Key B:
    unknown
```

Students first authenticate with the known key.

Then they measure:

```text
Authentication
↓
Session
↓
Nonce
↓
Crypto1
↓
Security analysis
```

The instructor can then demonstrate the difference between:

```text
Dictionary testing
```

and:

```text
Protocol-level attacks
```

This distinction is important because they are fundamentally different approaches.

---

# 📈 Performance Metrics

KeyLab records:

```text
Candidates tested
Authentication attempts
Successful authentications
Elapsed time
Average test rate
Reader errors
Card communication errors
```

Example:

```text
Performance
-----------

Candidates tested : 10,000
Successes          : 1
Failures           : 9,999

Elapsed             : 00:04:21
Average rate        : XXXX candidates/sec
```

---

# 🧪 Testing

Unit tests:

```bash
pytest
```

or:

```bash
npm test
```

depending on the implementation language.

Hardware tests should be separated from simulation tests:

```text
tests/
│
├── unit/
│
├── simulator/
│
└── hardware/
```

---

# 🛠️ Recommended Technology Stack

## Host Application

Recommended:

```text
Python 3.12+
```

Libraries:

```text
Typer
Rich
PySerial
Pydantic
```

Optional:

```text
Textual
FastAPI
```

---

## Embedded Firmware

For ESP32:

```text
C++
PlatformIO
Arduino Framework
```

Libraries:

```text
MFRC522
SPI
```

---

# 🖥️ Future GUI

A future GUI can be built using:

```text
Python
   │
   ├── PySide6
   │
   └── Qt
```

or:

```text
React
   │
   ▼
Local API
   │
   ▼
Python Backend
```

Example interface:

```text
┌─────────────────────────────────────────────┐
│              MIFARE KEYLAB                  │
├─────────────────────────────────────────────┤
│ Card: MIFARE Classic 1K                     │
│                                             │
│ UID: XX XX XX XX                            │
│                                             │
├─────────────────────────────────────────────┤
│ Sector        Key A          Key B           │
│                                             │
│ 0             ✓              ?              │
│ 1             ✓              ✓              │
│ 2             ?              ?              │
│ 3             ✓              ?              │
│                                             │
├─────────────────────────────────────────────┤
│ Candidate Testing                           │
│                                             │
│ Progress: ███████████░░░ 76%                │
│ Tested:   7600                              │
│                                             │
└─────────────────────────────────────────────┘
```

---

# 🔐 Security Design

The software should follow these principles:

1. No automatic targeting of arbitrary cards.
2. Explicit laboratory mode.
3. Candidate files must be explicitly supplied.
4. Unbounded brute-force searches are disabled by default.
5. Sensitive keys are hidden in output by default.
6. UID redaction is enabled in reports by default.
7. Hardware access requires explicit user interaction.
8. Simulation is available without physical hardware.

---

# 📦 Project Milestones

## v0.1

```text
[ ] Project structure
[ ] CLI
[ ] Card detection
[ ] Card information
```

## v0.2

```text
[ ] MIFARE Classic sectors
[ ] Authentication
[ ] Candidate file
[ ] Candidate validation
```

## v0.3

```text
[ ] Search-space generator
[ ] Progress UI
[ ] Reports
[ ] Logging
```

## v0.4

```text
[ ] Crypto1 simulator
[ ] PRNG visualization
[ ] Authentication visualization
```

## v0.5

```text
[ ] Nested Attack simulator
[ ] Security analysis
[ ] Classroom mode
```

## v1.0

```text
[ ] Chameleon Ultra backend
[ ] RC522 backend
[ ] PN532 backend
[ ] GUI
[ ] Documentation
```

---

# 📜 License

MIT License

This project is intended for educational and authorized security research.

---

# ⭐ Final Objective

MIFARE KeyLab should ultimately provide a complete educational environment:

```text
                 MIFARE KEYLAB
                       │
        ┌──────────────┼──────────────┐
        │              │              │
        ▼              ▼              ▼
      Reader       Authentication   Analysis
        │              │              │
        ▼              ▼              ▼
      RC522        Key A/B         Crypto1
      PN532        Sessions        PRNG
      Chameleon                    Nested
      Proxmark                      │
        │                           │
        └──────────────┬────────────┘
                       ▼
                Classroom Lab
                       │
                       ▼
              Security Understanding
```

The objective is not simply to find a key.

The objective is to understand **why the authentication system works, how its security assumptions fail, how those failures can be demonstrated in a controlled environment, and how modern systems avoid those weaknesses.**

