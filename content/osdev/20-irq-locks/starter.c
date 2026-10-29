#include <stdint.h>
#include <stddef.h>

typedef struct {
    volatile uint32_t locked;   // 0 = free, 1 = held
} SpinLock;

uint64_t read_flags(void) {
    return 0;
}

int irq_enabled(void) {
    return 0;
}

uint64_t irq_save(void) {
    return 0;
}

void irq_restore(uint64_t flags) {
}

uint64_t atomic_add(volatile uint64_t *p, uint64_t v) {
    return 0;
}

int atomic_cas(volatile uint64_t *p, uint64_t expected, uint64_t desired) {
    return 0;
}

void spin_init(SpinLock *l) {
}

int spin_trylock(SpinLock *l) {
    return 0;
}

void spin_lock(SpinLock *l) {
}

void spin_unlock(SpinLock *l) {
}

uint64_t spin_lock_irqsave(SpinLock *l) {
    return 0;
}

void spin_unlock_irqrestore(SpinLock *l, uint64_t flags) {
}

void irq_wait(void) {
}
