`user_range_ok`: `if (len == 0) return 1; if (ptr > USER_TOP) return 0; return len - 1 <= USER_TOP - ptr;`. Every subtraction there is safe because you checked the previous condition first.
===
The copy helpers are `if (!user_range_ok(...)) return -EFAULT;` followed by a byte loop. Casting the user address to a pointer is `(const uint8_t *)(uintptr_t)user_src`.
===
`sys_write` loop: `char kbuf[64]; while (len) { uint64_t n = len < 64 ? len : 64; copy_from_user(kbuf, buf + done, n); console_write(kbuf, n); done += n; len -= n; }`. Check the *whole* range with `user_range_ok` before the loop, so a bad buffer writes nothing at all.
===
Handlers all need the same signature to live in one table: `int64_t sys_getpid(uint64_t a, uint64_t b, uint64_t c) { (void)a; ... }`. Casting unused arguments to `void` silences the warning.
===
In `syscall_dispatch` test `num >= SYSCALL_MAX` first, then the null entry. Reading `syscall_table[num]` before checking the bound is the bug the lesson warns about.
