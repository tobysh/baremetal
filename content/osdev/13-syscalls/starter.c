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
    return 0;
}

int64_t copy_from_user(void *dst, uint64_t user_src, uint64_t len) {
    return -EFAULT;
}

int64_t copy_to_user(uint64_t user_dst, const void *src, uint64_t len) {
    return -EFAULT;
}

int64_t sys_write(uint64_t fd, uint64_t buf, uint64_t len) {
    return -ENOSYS;
}

int64_t sys_read(uint64_t fd, uint64_t buf, uint64_t len) {
    return -ENOSYS;
}

int64_t sys_getpid(uint64_t a1, uint64_t a2, uint64_t a3) {
    return -ENOSYS;
}

int64_t sys_exit(uint64_t code, uint64_t a2, uint64_t a3) {
    return -ENOSYS;
}

void syscall_init(void) {
}

int64_t syscall_dispatch(uint64_t num, uint64_t a1, uint64_t a2, uint64_t a3) {
    return -ENOSYS;
}
