#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// ---- Toy cipher API (given; deliberately weak, never use it for real) ----
typedef struct { uint64_t key; uint32_t counter; uint32_t limit; } Channel;
typedef struct { uint32_t nonce; size_t n; uint8_t ct[64]; uint64_t tag; } Packet;
typedef struct { char op; int key; uint32_t nonce; int aad_bound; int tag_first; } Call;

static uint64_t mix(uint64_t x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

// XOR the keystream for (key, nonce) into buf. The same call encrypts and decrypts.
void toy_xor(uint64_t key, uint32_t nonce, uint8_t *buf, size_t n) {
    uint64_t s = mix(key ^ ((uint64_t)nonce << 17));
    for (size_t i = 0; i < n; i++) { if (i % 8 == 0) s = mix(s); buf[i] ^= (uint8_t)(s >> (8 * (i % 8))); }
}

// Authentication tag over the key, nonce, the associated data (aad) and the ciphertext.
// Lengths are mixed in, so the parts cannot be shifted into each other.
uint64_t toy_tag(uint64_t key, uint32_t nonce, const char *aad, const uint8_t *ct, size_t n) {
    uint64_t h = mix(key ^ 0x5bd1e995ULL);
    h = mix(h ^ nonce);
    h = mix(h ^ strlen(aad));
    for (const char *p = aad; *p; p++) h = mix(h ^ (uint8_t)*p);
    h = mix(h ^ n);
    for (size_t i = 0; i < n; i++) h = mix(h ^ ct[i]);
    return h;
}
// ---- end of given code ----

// BROKEN: a fixed nonce, no limit, and the tag ignores the nonce and the aad.
int channel_seal(Channel *ch, const char *aad, const uint8_t *pt, size_t n, Packet *out) {
    (void)aad;
    if (n > 64) return -1;
    out->nonce = 1;
    out->n = n;
    memcpy(out->ct, pt, n);
    toy_xor(ch->key, out->nonce, out->ct, n);
    out->tag = toy_tag(ch->key, 0, "", out->ct, n);
    return 0;
}

// BROKEN: decrypts first, checks afterwards, and leaves the plaintext behind on failure.
int channel_open(const Channel *ch, const char *aad, const Packet *p, uint8_t *out, size_t outcap) {
    (void)aad;
    if (p->n > 64 || p->n > outcap) return -1;
    memcpy(out, p->ct, p->n);
    toy_xor(ch->key, p->nonce, out, p->n);
    if (toy_tag(ch->key, 0, "", p->ct, p->n) != p->tag) return -1;
    return (int)p->n;
}

int audit_calls(const Call *log, int n, int *kind) {
    (void)log; (void)n; *kind = 0;
    return -1;
}

int main(void) {
    Channel ch = { 0x1234, 0, 100 };
    Packet p;
    channel_seal(&ch, "user=7", (const uint8_t *)"hello", 5, &p);
    uint8_t out[16] = {0};
    int r = channel_open(&ch, "user=7", &p, out, sizeof out);
    printf("%d %.5s\n", r, (const char *)out);
    return 0;
}
