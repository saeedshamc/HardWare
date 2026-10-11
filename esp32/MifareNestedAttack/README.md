# MIFARE Classic Security Lab

> An educational RFID security laboratory for understanding MIFARE Classic, Crypto1 authentication, sector keys, access conditions, and the principles behind Nested Attacks using an ESP32 and MFRC522 (RC522).

---

## 📚 Overview

**MIFARE Classic Security Lab** is an educational hardware/software project designed to demonstrate how MIFARE Classic RFID cards work and why the original Crypto1-based security architecture is considered obsolete for security-sensitive applications.

The project is intended for:

* Cybersecurity education
* Embedded security courses
* RFID/NFC workshops
* Hardware security laboratories
* Cryptography demonstrations
* Reverse-engineering education
* IoT and embedded security training

The project uses:

```text
ESP32
   +
MFRC522 / RC522
   +
MIFARE Classic test card
```

The laboratory combines:

1. Real RFID communication
2. MIFARE Classic card inspection
3. Authentication demonstrations
4. Sector and block analysis
5. Crypto1 conceptual visualization
6. Nested Attack simulation
7. Defensive security analysis

---

# ⚠️ Educational / Ethical Use

This project is intended for:

* Cards owned by the instructor
* Cards specifically provided for laboratory exercises
* Test cards
* Laboratory environments
* Security research with explicit authorization

Do not use this project to access, modify, clone, or analyze cards that you do not own or have explicit permission to test.

For classroom demonstrations, use dedicated MIFARE Classic test cards.

---

# 🎯 Learning Objectives

After completing the laboratory, students should understand:

* What RFID is
* What ISO/IEC 14443 Type A means
* How a reader communicates with a MIFARE Classic card
* What UID, ATQA and SAK represent
* How MIFARE Classic memory is organized
* What sectors and blocks are
* What a sector trailer contains
* The purpose of Key A and Key B
* Access conditions
* The MIFARE Classic authentication process
* The role of Crypto1
* Why Crypto1 is no longer considered secure
* The basic idea behind Nested Attacks
* Why authenticated sessions can expose weaknesses in legacy designs
* Why modern systems should use stronger authentication mechanisms

---

# 🧠 MIFARE Classic Architecture

A typical MIFARE Classic 1K card contains:

```text
1 KB EEPROM
│
├── Sector 0
│   ├── Block 0
│   ├── Block 1
│   ├── Block 2
│   └── Block 3  ← Sector Trailer
│
├── Sector 1
│   ├── Block 4
│   ├── Block 5
│   ├── Block 6
│   └── Block 7  ← Sector Trailer
│
├── ...
│
└── Sector 15
    ├── Block 60
    ├── Block 61
    ├── Block 62
    └── Block 63 ← Sector Trailer
```

Each sector contains:

```text
Block 0
Block 1
Block 2
Block 3
```

The last block is called the **sector trailer**.

A sector trailer contains:

```text
┌─────────────────────────────────────────┐
│ Key A       │ Access Bits │ Key B       │
│ 6 bytes     │ 4 bytes     │ 6 bytes     │
└─────────────────────────────────────────┘
```

The exact access permissions depend on the access-condition bits.

MIFARE Classic EV1 1K is organized as 16 sectors with 4 blocks per sector and 16 bytes per block.

---

# 🔐 Crypto1

MIFARE Classic uses the proprietary **Crypto1** stream cipher for authentication and encrypted communication.

The basic communication flow is:

```text
Reader
   │
   │ Authentication Request
   ▼
Card
   │
   │ Random Challenge
   ▼
Reader
   │
   │ Authentication Response
   ▼
Card
   │
   │ Authentication Result
   ▼
Authenticated Session
```

After successful authentication, subsequent communication is encrypted according to the card's protocol state.

NXP documents this as a three-pass authentication process.

---

# 🧪 Laboratory Architecture

```text
                    ┌─────────────────────┐
                    │       Laptop        │
                    │                     │
                    │ Serial Monitor      │
                    │ Python Tools        │
                    │ Web Dashboard       │
                    └──────────┬──────────┘
                               │ USB
                               │
                    ┌──────────▼──────────┐
                    │        ESP32        │
                    │                     │
                    │ MIFARE Lab Engine   │
                    └──────────┬──────────┘
                               │ SPI
                               │
                    ┌──────────▼──────────┐
                    │       MFRC522       │
                    │       RC522         │
                    │                     │
                    │ 13.56 MHz RFID      │
                    └──────────┬──────────┘
                               │
                         RF interface
                               │
                    ┌──────────▼──────────┐
                    │  MIFARE Classic     │
                    │      Test Card      │
                    └─────────────────────┘
```

---

# 🧰 Hardware

## Required

### ESP32

Recommended:

* ESP32 DevKit V1
* ESP32-WROOM based boards

Other supported platforms may include:

* Arduino Nano
* Arduino Pro Mini
* ESP8266
* Luckfox boards with suitable SPI support

However, ESP32 is the recommended platform for this project.

---

## RC522 / MFRC522

The project uses an MFRC522-based RFID reader.

Typical modules expose:

```text
SDA / SS
SCK
MOSI
MISO
IRQ
GND
RST
3.3V
```

The microcontroller communicates with the RC522 over SPI.

---

# 🔌 ESP32 Wiring

Typical ESP32 VSPI configuration:

| RC522    | ESP32        |
| -------- | ------------ |
| 3.3V     | 3.3V         |
| GND      | GND          |
| SDA / SS | GPIO 5       |
| SCK      | GPIO 18      |
| MOSI     | GPIO 23      |
| MISO     | GPIO 19      |
| RST      | GPIO 22      |
| IRQ      | Not required |

```text
             ESP32
        ┌─────────────┐
        │             │
 GPIO 5  ├─────────────┤ SDA/SS
 GPIO 18 ├─────────────┤ SCK
 GPIO 23 ├─────────────┤ MOSI
 GPIO 19 ├─────────────┤ MISO
 GPIO 22 ├─────────────┤ RST
 3.3V    ├─────────────┤ 3.3V
 GND     ├─────────────┤ GND
        │             │
        └─────────────┘
              │
              │ SPI
              ▼
        ┌─────────────┐
        │   RC522     │
        └─────────────┘
```

> **Important:** Use 3.3V for the RC522 module. Do not power the module from 5V unless the specific board explicitly supports it.

---

# 💻 Software Requirements

Recommended:

* Arduino IDE
* PlatformIO
* VS Code
* ESP32 board package
* MFRC522 Arduino library

A commonly used MFRC522 library supports MIFARE Classic 1K/4K/Mini communication and Crypto1 authentication.

---

# 📦 Project Structure

```text
mifare-security-lab/
│
├── README.md
├── LICENSE
├── CONTRIBUTING.md
├── SECURITY.md
│
├── firmware/
│   │
│   ├── esp32/
│   │   ├── platformio.ini
│   │   └── src/
│   │       ├── main.cpp
│   │       ├── rfid/
│   │       │   ├── rfid_reader.cpp
│   │       │   ├── rfid_reader.h
│   │       │   ├── mifare.cpp
│   │       │   └── mifare.h
│   │       │
│   │       ├── security/
│   │       │   ├── authentication.cpp
│   │       │   ├── authentication.h
│   │       │   └── crypto_demo.cpp
│   │       │
│   │       └── cli/
│   │           ├── cli.cpp
│   │           └── cli.h
│   │
│   └── arduino/
│       └── ...
│
├── simulator/
│   │
│   ├── crypto1/
│   │   ├── README.md
│   │   ├── prng_demo.py
│   │   └── authentication_demo.py
│   │
│   └── nested/
│       ├── README.md
│       └── nested_simulator.py
│
├── tools/
│   ├── card_info/
│   ├── sector_analyzer/
│   └── access_bits/
│
├── examples/
│   ├── card_info/
│   ├── authentication/
│   └── sector_analysis/
│
├── docs/
│   ├── architecture.md
│   ├── mifare-classic.md
│   ├── crypto1.md
│   ├── nested-attack.md
│   └── laboratory.md
│
└── tests/
    ├── unit/
    └── simulator/
```

---

# 🚀 Getting Started

## 1. Clone the repository

```bash
git clone https://github.com/YOUR_USERNAME/mifare-security-lab.git
cd mifare-security-lab
```

---

# 2. Install PlatformIO

Install PlatformIO through VS Code or use the PlatformIO CLI.

Then verify:

```bash
pio --version
```

---

# 3. Connect ESP32

Connect:

```text
ESP32 <-> RC522
```

according to the wiring table above.

---

# 4. Build

```bash
pio run
```

---

# 5. Upload

```bash
pio run --target upload
```

---

# 6. Open Serial Monitor

```bash
pio device monitor
```

Typical output:

```text
========================================
      MIFARE SECURITY LAB
========================================

[+] Initializing SPI...
[+] Initializing MFRC522...
[+] Reader detected.

Waiting for card...
```

---

# 🪪 Module 01 — Card Detection

The first module detects a card and displays:

```text
UID
ATQA
SAK
Card Type
```

Example:

```text
[+] Card detected

UID  : 04 A1 B2 C3 D4 55 66
ATQA : 00 44
SAK  : 08

Card:
MIFARE Classic compatible
```

This module does not attempt authentication.

---

# 🔍 Module 02 — Card Information

The tool can display:

```text
Card Information
---------------------------
UID
ATQA
SAK
Card Type
Memory Size
Sector Count
Block Count
```

For MIFARE Classic 1K:

```text
Memory     : 1024 bytes
Sectors    : 16
Blocks     : 64
Block size : 16 bytes
```

---

# 🔐 Module 03 — Authentication Demonstration

The laboratory provides an educational authentication demonstration.

Example:

```text
[+] Card selected

[*] Authenticating Sector 1
[*] Block: 4
[*] Authentication: KEY_A

[+] Authentication successful

[*] Secure communication state established
```

The program should clearly distinguish:

```text
Authentication SUCCESS
```

from:

```text
Authentication FAILURE
```

---

# 🧱 Module 04 — Sector Analyzer

The sector analyzer displays the logical structure of a sector.

Example:

```text
Sector 01
────────────────────────────────

Block 04
DATA

Block 05
DATA

Block 06
DATA

Block 07
SECTOR TRAILER

Key A       : [REDACTED]
Access Bits : FF 07 80
Key B       : [REDACTED]
```

The default UI should **not print secret keys** unless the instructor explicitly enables a controlled laboratory mode.

---

# 🧬 Module 05 — Access Conditions

The project includes an access-condition decoder.

Example:

```text
Sector Trailer

Access Bits:
FF 07 80

Block 0:
Read  : allowed
Write : allowed

Block 1:
Read  : allowed
Write : restricted

Block 2:
Read  : allowed
Write : restricted

Trailer:
Key A access : restricted
Key B access : restricted
```

This allows students to understand that authentication alone is not the entire security model.

---

# 🔬 Module 06 — Crypto1 Visualization

This module demonstrates the concept of a stream cipher.

Conceptual model:

```text
Secret Key
    │
    ▼
┌───────────────┐
│ Crypto1 State │
└───────┬───────┘
        │
        ▼
  Keystream
        │
        ├──────────────┐
        ▼              ▼
 Plaintext         Ciphertext
```

The simulator visualizes:

* key material
* nonce
* pseudo-random state
* keystream
* encrypted communication

The goal is to explain the protocol rather than expose real-world credentials.

---

# 🧪 Module 07 — Nested Attack Simulator

The Nested Attack simulator is intentionally implemented as a **controlled educational model**.

The simulation demonstrates:

```text
Known Authentication
        │
        ▼
Authenticated Session
        │
        ▼
Crypto1 State Relationship
        │
        ▼
Second Authentication
        │
        ▼
Information Leakage
```

Example:

```text
========================================
       NESTED ATTACK SIMULATOR
========================================

[1] Load known authentication
[2] Generate card nonce
[3] Establish Crypto1 state
[4] Start nested authentication
[5] Analyze leaked information
[6] Visualize attack concept

Select:
```

The simulator can show students how a weakness in the relationship between authentication sessions can make legacy systems vulnerable.

---

# 🧠 Why Nested Attacks Work Conceptually

The important lesson is:

> A secure-looking authentication protocol can still be vulnerable if the underlying cryptographic state and nonce generation allow information from one authenticated session to help analyze another session.

Conceptually:

```text
Authentication #1
        │
        ▼
 ┌─────────────┐
 │ Crypto State│
 └──────┬──────┘
        │
        │ relationship
        ▼
Authentication #2
        │
        ▼
Information
Leakage
```

This is why simply saying:

```text
"We use encryption"
```

does not automatically mean:

```text
"The system is secure."
```

---

# 🧑‍🏫 Suggested Classroom Demonstration

## Part 1 — Identify the card

Students place the card on the RC522.

The system displays:

```text
UID
ATQA
SAK
Card type
```

---

## Part 2 — Explain memory

Show:

```text
Sector 0
Sector 1
Sector 2
...
Sector 15
```

Then explain:

```text
Data Blocks
     +
Sector Trailer
     =
Sector
```

---

## Part 3 — Authentication

Demonstrate authentication using a key that is intentionally known for the laboratory card.

```text
Reader
   │
   │ AUTH
   ▼
Card
   │
   │ Challenge
   ▼
Reader
   │
   │ Response
   ▼
Card
   │
   ▼
Authenticated
```

---

## Part 4 — Explain Crypto1

Introduce:

```text
Key
 +
Nonce
 +
Internal State
       │
       ▼
   Keystream
```

Then explain why the security of the whole system depends on the security of this process.

---

## Part 5 — Nested Concept

Switch to the simulator.

Show:

```text
Known Key
   │
   ▼
Authentication
   │
   ▼
Crypto1 State
   │
   ▼
Second Authentication
   │
   ▼
Information Leakage
```

Students can now understand what a Nested Attack is without treating the card as a black box.

---

# 🧪 Real Card Mode

The project supports **real MIFARE Classic test cards** for:

* card detection
* UID/ATQA/SAK inspection
* sector identification
* authentication using known laboratory keys
* authorized memory access
* access-condition analysis

The RC522 library supports Crypto1 communication with MIFARE Classic cards.

However, the RC522 is not equivalent to a Proxmark3.

The hardware/software stack has important limitations.

---

# ⚠️ RC522 vs Proxmark3

```text
                  RC522             Proxmark3
-------------------------------------------------
13.56 MHz RFID       ✓                   ✓
MIFARE Classic       ✓                   ✓
Authentication       ✓                   ✓
Read/Write           ✓                   ✓
Crypto1 support      ✓                   ✓
Raw RF analysis      limited             ✓
Advanced sniffing    limited             ✓
Protocol research    limited             ✓
Flexible RF control  limited             ✓
Advanced attacks     limited             ✓
```

The MFRC522 is designed as a reader/writer IC, while Proxmark3 is a much more general RFID/NFC research platform.

---

# 🔴 Important Limitation

This project does **not** attempt to turn the RC522 into a full Proxmark3 replacement.

In particular, the RC522 is not intended to provide the same low-level RF capture and protocol manipulation capabilities.

Therefore:

```text
RC522 + ESP32
```

is excellent for:

```text
Learning
↓
Authentication
↓
Memory structure
↓
Crypto1 concepts
↓
Security demonstrations
```

but is not a drop-in replacement for:

```text
Proxmark3
```

for advanced RF research.

---

# 🛡️ Defensive Security Module

The final laboratory module demonstrates how to design systems that do not depend on MIFARE Classic Crypto1 as the primary security mechanism.

Students compare:

```text
Legacy Architecture
────────────────────

MIFARE Classic
      │
      ▼
   Crypto1
      │
      ▼
Access Control
```

with:

```text
Modern Architecture
────────────────────

Secure Card
      │
      ▼
Modern Authentication
      │
      ▼
Session Security
      │
      ▼
Backend Authorization
```

NXP itself recommends newer MIFARE families such as DESFire and MIFARE Plus for security-relevant applications.

---

# 🧪 Recommended Test Cards

For classroom work, use dedicated MIFARE Classic cards.

Recommended setup:

```text
Card A
Known test keys
     │
     └── Main laboratory card

Card B
Different keys
     │
     └── Authentication experiment

Card C
Intentionally configured weak permissions
     │
     └── Access-condition experiment
```

Never use:

* access cards belonging to other people
* university access cards
* workplace access cards
* public transport cards
* payment-related cards
* cards without explicit authorization

---

# 📊 Example Serial Interface

```text
========================================
 MIFARE CLASSIC SECURITY LAB
========================================

1. Detect Card
2. Card Information
3. Authenticate
4. Read Authorized Block
5. Sector Analyzer
6. Access Bits Decoder
7. Crypto1 Demonstration
8. Nested Attack Simulator
9. Defensive Analysis
0. Exit

Select:
```

---

# 🧩 Configuration

Example configuration:

```cpp
#define RFID_SS_PIN   5
#define RFID_RST_PIN  22

#define RFID_SCK_PIN  18
#define RFID_MISO_PIN 19
#define RFID_MOSI_PIN 23
```

---

# 🔧 Development Roadmap

## Phase 1

```text
[ ] ESP32 setup
[ ] RC522 SPI communication
[ ] Card detection
[ ] UID
[ ] ATQA
[ ] SAK
```

## Phase 2

```text
[ ] MIFARE Classic detection
[ ] Sector mapping
[ ] Block mapping
[ ] Authentication demo
[ ] Authorized read/write
```

## Phase 3

```text
[ ] Sector trailer parser
[ ] Access condition decoder
[ ] Authentication visualization
```

## Phase 4

```text
[ ] Crypto1 educational simulator
[ ] PRNG visualization
[ ] Authentication-state visualization
[ ] Nested Attack simulator
```

## Phase 5

```text
[ ] Web dashboard
[ ] Live serial log
[ ] Graphical card map
[ ] Authentication timeline
[ ] Classroom presentation mode
```

---

# 🌐 Future Web Dashboard

The ESP32 can eventually expose a local web interface:

```text
http://mifare-lab.local
```

Example:

```text
┌──────────────────────────────────────────┐
│          MIFARE SECURITY LAB             │
├──────────────────────────────────────────┤
│                                          │
│ Card Status:       CONNECTED             │
│ Card Type:         MIFARE Classic 1K     │
│ UID:               XX XX XX XX           │
│                                          │
├──────────────────────────────────────────┤
│ Sector Map                               │
│                                          │
│ Sector 0      ████████                   │
│ Sector 1      ████████                   │
│ Sector 2      ████████                   │
│ Sector 3      ████████                   │
│ ...                                      │
│                                          │
├──────────────────────────────────────────┤
│ Authentication                           │
│                                          │
│ Status: SUCCESS                          │
│ Key Type: KEY A                          │
│ Block: 4                                 │
│                                          │
└──────────────────────────────────────────┘
```

---

# 🧑‍💻 Development Philosophy

The project follows three principles:

### 1. Hardware First

Students should first understand the physical communication:

```text
13.56 MHz
   ↓
RF Field
   ↓
Card
   ↓
Reader
   ↓
SPI
   ↓
Microcontroller
```

### 2. Protocol Before Attack

Before demonstrating a security weakness:

```text
Understand the protocol
        ↓
Understand authentication
        ↓
Understand cryptographic state
        ↓
Understand the weakness
        ↓
Demonstrate the weakness
```

### 3. Offensive + Defensive

Every attack demonstration should have a corresponding defensive lesson.

```text
Attack
  ↓
Why did it work?
  ↓
What design decision caused it?
  ↓
How can we prevent it?
```

---

# 📚 Recommended Documentation

Official references:

* NXP MIFARE Classic documentation
* NXP MFRC522 datasheet
* ISO/IEC 14443 Type A documentation
* MFRC522 library documentation

MIFARE Classic EV1 documentation describes the memory organization, sector keys, access conditions, and three-pass authentication.

---

# 📜 License

Choose a license appropriate for your project.

Recommended:

```text
MIT License
```

with an additional educational-use notice in:

```text
SECURITY.md
```

---

# ⭐ Project Goal

The goal of this project is not simply to create another RFID tool.

The goal is to make RFID security understandable.

A student should finish the laboratory being able to answer:

> What happens when I place a MIFARE Classic card next to an RFID reader?

and then progressively understand:

```text
RF
│
├── ISO/IEC 14443
│
├── Anticollision
│
├── Card Selection
│
├── Authentication
│
├── Crypto1
│
├── Memory
│
├── Access Conditions
│
├── Security Weaknesses
│
└── Modern Secure Alternatives
```

---

## Project Status

🚧 **Early Development**

Current target:

```text
ESP32
+
MFRC522
+
MIFARE Classic
```

Future target:

```text
ESP32
+
RC522
+
Web Dashboard
+
Crypto1 Simulator
+
Interactive Nested Attack Demonstration
```

