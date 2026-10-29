#include <stdint.h>
typedef struct { int held; } Spinlock;
static unsigned long irq_save(void) { unsigned long f; __asm__ volatile("pushfq; popq %0; cli" : "=r"(f) : : "memory"); return f; }
static void irq_restore(unsigned long f) { if(f & 0x200) __asm__ volatile("sti" ::: "memory"); }
unsigned long lock_irqsave(Spinlock *l) { unsigned long f=irq_save(); while(__atomic_exchange_n(&l->held,1,__ATOMIC_ACQUIRE)) __asm__ volatile("pause"); return f; }
void unlock_irqrestore(Spinlock *l, unsigned long f) { __atomic_store_n(&l->held,0,__ATOMIC_RELEASE); irq_restore(f); }
