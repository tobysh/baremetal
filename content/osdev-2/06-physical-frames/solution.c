#include <stdint.h>

#define BASE 0x100000ULL   /* address of frame 0 */
#define FRAMES 64          /* frames in the pool */
#define FRAME_SIZE 4096ULL

/* Bit i set: frame i is taken or reserved. Bit i clear: frame i is free. */
static uint64_t used;

void frame_init(void) {
    used = 0;
}

void frame_reserve(unsigned first, unsigned count) {
    if (first >= FRAMES)
        return;                         /* nothing of the range is in the pool */
    if (count > FRAMES - first)
        count = FRAMES - first;         /* clamp without computing first + count */
    for (unsigned i = first; i < first + count; i++)
        used |= 1ULL << i;
}

uint64_t frame_take(void) {
    for (unsigned i = 0; i < FRAMES; i++) {
        if (!(used & (1ULL << i))) {
            used |= 1ULL << i;          /* claim it before handing it out */
            return BASE + FRAME_SIZE * i;
        }
    }
    return 0;                           /* pool exhausted */
}

int frame_release(uint64_t addr) {
    if (addr < BASE || addr >= BASE + FRAMES * FRAME_SIZE)
        return -1;                      /* outside the pool */
    if ((addr - BASE) % FRAME_SIZE)
        return -1;                      /* not the start of a frame */
    unsigned i = (unsigned)((addr - BASE) / FRAME_SIZE);
    if (!(used & (1ULL << i)))
        return -1;                      /* already free: double free */
    used &= ~(1ULL << i);
    return 0;
}
