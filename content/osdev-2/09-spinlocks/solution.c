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
    unsigned long flags = irq_save();   /* no interrupt can arrive from here on */
    while (__atomic_exchange_n(&l->held, 1, __ATOMIC_ACQUIRE))
        __asm__ volatile("pause");      /* someone else holds it: wait politely */
    return flags;
}

void unlock_irqrestore(Spinlock *l, unsigned long flags) {
    __atomic_store_n(&l->held, 0, __ATOMIC_RELEASE);
    irq_restore(flags);                 /* only after the lock is free */
}
