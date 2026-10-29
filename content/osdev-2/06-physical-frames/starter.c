#include <stdint.h>
#define BASE 0x100000ULL
static uint64_t used;
void frame_init(void) {}
void frame_reserve(unsigned first, unsigned count) {}
uint64_t frame_take(void) { return 0; }
int frame_release(uint64_t addr) { return -1; }
