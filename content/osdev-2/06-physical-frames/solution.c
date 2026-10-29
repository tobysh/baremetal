#include <stdint.h>
#define BASE 0x100000ULL
static uint64_t used;
void frame_init(void) { used = 0; }
void frame_reserve(unsigned first, unsigned count) { if (first >= 64) return; if (count > 64-first) count=64-first; for (unsigned i=first;i<first+count;i++) used |= 1ULL<<i; }
uint64_t frame_take(void) { for (unsigned i=0;i<64;i++) if (!(used & (1ULL<<i))) { used |= 1ULL<<i; return BASE+4096ULL*i; } return 0; }
int frame_release(uint64_t a) { if (a<BASE || a>=BASE+64*4096ULL || (a-BASE)%4096) return -1; unsigned i=(a-BASE)/4096; if (!(used & (1ULL<<i))) return -1; used &= ~(1ULL<<i); return 0; }
