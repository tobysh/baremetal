#include <stdint.h>
#include <stddef.h>

#define EBADF  9
#define EFAULT 14
#define EINVAL 22
#define ENOSYS 38

#define USER_TOP        0x00007FFFFFFFFFFFULL
#define SYSCALL_MAX     64

// ---- The rest of the OS (provided) ----
char console_log[1024];
size_t console_len, console_max_chunk, console_calls;

void console_write(const char *s, size_t n) {
    console_calls++;
    if (n > console_max_chunk) console_max_chunk = n;
    for (size_t i = 0; i < n && console_len < sizeof console_log; i++) console_log[console_len++] = s[i];
}

const char *input_data = "hello";   // what the keyboard has typed so far
size_t input_pos;                    // how much of it has been read
int current_pid = 42;
int exit_code;
int exited;

// ---- Your code ----

typedef int64_t (*syscall_fn)(uint64_t a1, uint64_t a2, uint64_t a3);
static syscall_fn syscall_table[SYSCALL_MAX];

int user_range_ok(uint64_t ptr, uint64_t len) {
    if (len == 0) return 1;
    if (ptr > USER_TOP) return 0;
    return len - 1 <= USER_TOP - ptr;   // cannot wrap: ptr <= USER_TOP here
}

int64_t copy_from_user(void *dst, uint64_t user_src, uint64_t len) {
    if (!user_range_ok(user_src, len)) return -EFAULT;
    const uint8_t *s = (const uint8_t *)(uintptr_t)user_src;
    uint8_t *d = dst;
    for (uint64_t i = 0; i < len; i++) d[i] = s[i];
    return 0;
}

int64_t copy_to_user(uint64_t user_dst, const void *src, uint64_t len) {
    if (!user_range_ok(user_dst, len)) return -EFAULT;
    uint8_t *d = (uint8_t *)(uintptr_t)user_dst;
    const uint8_t *s = src;
    for (uint64_t i = 0; i < len; i++) d[i] = s[i];
    return 0;
}

int64_t sys_write(uint64_t fd, uint64_t buf, uint64_t len) {
    if (fd != 1 && fd != 2) return -EBADF;
    if (!user_range_ok(buf, len)) return -EFAULT;
    char kbuf[64];
    uint64_t done = 0;
    while (done < len) {
        uint64_t n = len - done < sizeof kbuf ? len - done : sizeof kbuf;
        copy_from_user(kbuf, buf + done, n);
        console_write(kbuf, n);
        done += n;
    }
    return (int64_t)len;
}

int64_t sys_read(uint64_t fd, uint64_t buf, uint64_t len) {
    if (fd != 0) return -EBADF;
    if (!user_range_ok(buf, len)) return -EFAULT;
    size_t total = 0;
    while (input_data[total]) total++;
    uint64_t left = total - input_pos;
    uint64_t n = len < left ? len : left;
    if (copy_to_user(buf, input_data + input_pos, n) < 0) return -EFAULT;
    input_pos += n;
    return (int64_t)n;
}

int64_t sys_getpid(uint64_t a1, uint64_t a2, uint64_t a3) {
    (void)a1; (void)a2; (void)a3;
    return current_pid;
}

int64_t sys_exit(uint64_t code, uint64_t a2, uint64_t a3) {
    (void)a2; (void)a3;
    exit_code = (int)code;
    exited = 1;
    return 0;
}

void syscall_init(void) {
    for (int i = 0; i < SYSCALL_MAX; i++) syscall_table[i] = NULL;
    syscall_table[0] = sys_read;
    syscall_table[1] = sys_write;
    syscall_table[39] = sys_getpid;
    syscall_table[60] = sys_exit;
}

int64_t syscall_dispatch(uint64_t num, uint64_t a1, uint64_t a2, uint64_t a3) {
    if (num >= SYSCALL_MAX || !syscall_table[num]) return -ENOSYS;
    return syscall_table[num](a1, a2, a3);
}
