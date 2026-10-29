#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#define HDR_LEN 10
#define HDR_MAGIC 0xB4DE

typedef struct {
    uint8_t version;  // 4 bits
    uint8_t flags;    // 4 bits
    uint8_t type;
    uint16_t length;
    uint32_t seq;
} Header;

static inline void put_u16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v; }
static inline void put_u32(uint8_t *p, uint32_t v) { put_u16(p, (uint16_t)(v >> 16)); put_u16(p + 2, (uint16_t)v); }
static inline uint16_t get_u16(const uint8_t *p) { return (uint16_t)((p[0] << 8) | p[1]); }
static inline uint32_t get_u32(const uint8_t *p) { return ((uint32_t)get_u16(p) << 16) | get_u16(p + 2); }

void hdr_encode(const Header *h, uint8_t out[10]) {
}

int hdr_decode(const uint8_t *buf, size_t n, Header *h) {
    return -2;
}

// Optional: try them with Run.
int main(void) {
    Header h = {1, 5, 7, 300, 99};
    uint8_t buf[HDR_LEN];
    hdr_encode(&h, buf);
    for (int i = 0; i < HDR_LEN; i++) printf("%02x ", buf[i]);
    printf("\n");
    return 0;
}
