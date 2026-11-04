#include "crypto/crypto1.h"
#include <string.h>

// ─── Constants ─────────────────────────────────────────────────────────────
#define MASK48   0xFFFFFFFFFFFFULL
#define B(x,n)   (((uint64_t)(x)>>(n))&1ULL)

// Feedback poly taps  (bit 0 = oldest / output end):
// {0,5,6,7,9,13,19,21,23,24,29,31,33,34,36,38,39,43}
static const uint64_t LFSR_POLY =
    (1ULL<<0)|(1ULL<<5)|(1ULL<<6)|(1ULL<<7)|(1ULL<<9)|(1ULL<<13)|
    (1ULL<<19)|(1ULL<<21)|(1ULL<<23)|(1ULL<<24)|(1ULL<<29)|(1ULL<<31)|
    (1ULL<<33)|(1ULL<<34)|(1ULL<<36)|(1ULL<<38)|(1ULL<<39)|(1ULL<<43);

// ─── Filter helpers ────────────────────────────────────────────────────────
// fa:  (0x9E98 >> ((d<<3)|(c<<2)|(b<<1)|a)) & 1   [verified vs. paper]
// fb:  (0xD71B824E >> ((e<<4)|(d<<3)|(c<<2)|(b<<1)|a)) & 1
//      derived from f_b(a,b,c,d,e) = (a|b)^(a&c)^(b&d)^(a&e)^e  — balanced (16/32)
static inline uint8_t _fa(uint8_t a,uint8_t b,uint8_t c,uint8_t d){
    return (0x9E98U>>((d<<3)|(c<<2)|(b<<1)|a))&1;
}
static inline uint8_t _fb(uint8_t a,uint8_t b,uint8_t c,uint8_t d,uint8_t e){
    return (0xD71B824EUL>>((e<<4)|(d<<3)|(c<<2)|(b<<1)|a))&1;
}

// ─── Filter (uses LFSR state bits at odd positions 9..41) ──────────────────
uint8_t crypto1_filter(uint64_t x){
    return _fb(
        _fa(B(x,9), B(x,11),B(x,13),B(x,15)),
        _fa(B(x,17),B(x,19),B(x,21),B(x,23)),
        _fa(B(x,25),B(x,27),B(x,29),B(x,31)),
        _fa(B(x,33),B(x,35),B(x,37),B(x,39)),
        (uint8_t)B(x,41)
    );
}

static inline uint8_t _feedback(uint64_t st){
    return (uint8_t)__builtin_parityll(st&LFSR_POLY);
}

// ─── Key loading (MSB-first, 48 bits) ─────────────────────────────────────
void crypto1_init(Crypto1* s, const uint8_t* key6){
    s->state=0;
    for(int i=0;i<6;i++) for(int b=7;b>=0;b--){
        s->state=((s->state>>1)&MASK48)|((uint64_t)((key6[i]>>b)&1)<<47);
    }
}

// ─── Forward clock ─────────────────────────────────────────────────────────
uint8_t crypto1_bit(Crypto1* s, uint8_t in, bool fb){
    uint8_t ks=crypto1_filter(s->state);
    uint8_t f =_feedback(s->state)^(fb?in:0);
    s->state=((s->state>>1)&MASK48)|((uint64_t)f<<47);
    return ks;
}
uint8_t crypto1_byte(Crypto1* s, uint8_t in, bool fb){
    uint8_t o=0; for(int i=7;i>=0;i--) o|=(uint8_t)crypto1_bit(s,(in>>i)&1,fb)<<i; return o;
}
uint32_t crypto1_word(Crypto1* s, uint32_t in, bool fb){
    uint32_t o=0; for(int i=31;i>=0;i--) o|=(uint32_t)crypto1_bit(s,(in>>i)&1,fb)<<i; return o;
}

// ─── Rollback ──────────────────────────────────────────────────────────────
// Forward: new = (old>>1)|(feedback<<47)   feedback=parity(old&POLY)^in
// Inverse: base=(new<<1)&MASK48  (places new[0..46] at old[1..47], bit0=0)
//          old[0] = new[47] ^ in ^ parity(base&POLY)   (unique since POLY has bit0)
uint8_t crypto1_rollback_bit(Crypto1* s, uint8_t inputBit){
    uint64_t base=(s->state<<1)&MASK48;
    uint8_t b0=(uint8_t)B(s->state,47)^inputBit^(uint8_t)__builtin_parityll(base&LFSR_POLY);
    s->state=base|(uint64_t)b0;
    return crypto1_filter(s->state);
}

// ─── MIFARE PRNG (verified against mfoc) ───────────────────────────────────
static inline uint32_t _sw(uint32_t x){
    return((x&0xFF)<<24)|((x&0xFF00)<<8)|((x&0xFF0000)>>8)|((x>>24)&0xFF);
}
uint32_t crypto1_prng_successor(uint32_t x, uint32_t n){
    x=_sw(x); while(n--) x=x>>1|(x^x>>2^x>>3^x>>5)<<31; return _sw(x);
}

// ─── Nonce verification ────────────────────────────────────────────────────
bool crypto1_verify_nonce(uint64_t key48, uint32_t uid, uint32_t nT, uint32_t nR, uint32_t aR){
    uint8_t key[6]; for(int i=5;i>=0;i--){key[i]=(uint8_t)(key48&0xFF);key48>>=8;}
    Crypto1 s; crypto1_init(&s,key);
    crypto1_word(&s,uid,false); crypto1_word(&s,nT,false); crypto1_word(&s,nR,true);
    return aR==(crypto1_word(&s,0,false)^crypto1_prng_successor(nT,64));
}
