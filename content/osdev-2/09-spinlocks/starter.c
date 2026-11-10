#include <stdint.h>

typedef struct {
    int held;     /* 0: free, 1: taken */
} Spinlock;

#define RFLAGS_IF 0x200UL   /* bit 9: interrupts enabled */

/* Returns RFLAGS as it was, then disables interrupts. */
static unsigned long irq_save(void) {
    unsigned long f;
    __asm__ volatile("pushfq; popq %0; cli" : "=r"(f) : : "memory");
    return f;
}

/* Re-enables interrupts only if they were enabled when f was saved. */
static void irq_restore(unsigned long f) {
    if (f & RFLAGS_IF)
        __asm__ volatile("sti" ::: "memory");
}

unsigned long lock_irqsave(Spinlock *l) {
    /* TODO: disable interrupts, then take the lock atomically */
    (void)l;
    return 0;
}

void unlock_irqrestore(Spinlock *l, unsigned long flags) {
    /* TODO: release the lock, then restore the caller's interrupt state */
    (void)l;
    (void)flags;
}
