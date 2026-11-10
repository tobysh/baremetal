#include <stdint.h>

#define BASE 0x100000ULL   /* address of frame 0 */
#define FRAMES 64          /* frames in the pool */
#define FRAME_SIZE 4096ULL

/* Bit i set: frame i is taken or reserved. Bit i clear: frame i is free. */
static uint64_t used;

void frame_init(void) {
    /* TODO: every frame starts free */
}

void frame_reserve(unsigned first, unsigned count) {
    /* TODO: mark [first, first + count) clamped to the pool.
       Careful: first + count can overflow an unsigned. */
    (void)first;
    (void)count;
}

uint64_t frame_take(void) {
    /* TODO: lowest free frame, mark it, return its address; 0 when full */
    return 0;
}

int frame_release(uint64_t addr) {
    /* TODO: validate range, alignment and current state before clearing */
    (void)addr;
    return -1;
}
