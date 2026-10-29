#include <stdint.h>
#include <stddef.h>

typedef struct {
    volatile uint32_t locked;   // 0 = free, 1 = held
} SpinLock;

#define FLAG_IF (1ull << 9)

uint64_t read_flags(void) {
    uint64_t f;
    __asm__ volatile("pushfq; popq %0" : "=r"(f));
    return f;
}

int irq_enabled(void) {
    return (read_flags() & FLAG_IF) != 0;
}

uint64_t irq_save(void) {
    uint64_t f = read_flags();
    __asm__ volatile("cli" : : : "memory");
    return f;
}

void irq_restore(uint64_t flags) {
    if (flags & FLAG_IF) __asm__ volatile("sti" : : : "memory");
}

uint64_t atomic_add(volatile uint64_t *p, uint64_t v) {
    __asm__ volatile("lock xaddq %0, %1" : "+r"(v), "+m"(*p) : : "memory");
    return v;
}

int atomic_cas(volatile uint64_t *p, uint64_t expected, uint64_t desired) {
    return __atomic_compare_exchange_n(p, &expected, desired, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

void spin_init(SpinLock *l) {
    __atomic_store_n(&l->locked, 0, __ATOMIC_RELEASE);
}

int spin_trylock(SpinLock *l) {
    return __atomic_exchange_n(&l->locked, 1, __ATOMIC_ACQUIRE) == 0;
}

void spin_lock(SpinLock *l) {
    while (!spin_trylock(l)) {
        __asm__ volatile("pause");
    }
}

void spin_unlock(SpinLock *l) {
    __atomic_store_n(&l->locked, 0, __ATOMIC_RELEASE);
}

uint64_t spin_lock_irqsave(SpinLock *l) {
    uint64_t flags = irq_save();
    spin_lock(l);
    return flags;
}

void spin_unlock_irqrestore(SpinLock *l, uint64_t flags) {
    spin_unlock(l);
    irq_restore(flags);
}

void irq_wait(void) {
    __asm__ volatile("sti; hlt" : : : "memory");
}
