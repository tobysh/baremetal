#define P(x) ((uint64_t)(uintptr_t)(x))
#define KERNEL_PTR 0xFFFF800000001000ULL

static void reset_os(void) {
    console_len = 0; console_max_chunk = 0; console_calls = 0;
    memset(console_log, 0, sizeof console_log);
    input_pos = 0; exited = 0; exit_code = 0;
}

void kmain(void) {
    CHECK("user_range_ok accepts an ordinary range", user_range_ok(0x1000, 0x100) == 1, "should be 1");
    CHECK("it accepts the very last byte of user space", user_range_ok(USER_TOP, 1) == 1, "should be 1");
    CHECK("it rejects a range that runs one byte past the end", user_range_ok(USER_TOP, 2) == 0 && user_range_ok(USER_TOP - 9, 11) == 0, "should be 0");
    CHECK("it accepts a range ending exactly at the last byte", user_range_ok(USER_TOP - 9, 10) == 1, "should be 1");
    CHECK("it rejects pointers into kernel space", user_range_ok(KERNEL_PTR, 1) == 0 && user_range_ok(USER_TOP + 1, 1) == 0, "should be 0");
    CHECK("it is not fooled by ptr + len wrapping around", user_range_ok(0xFFFFFFFFFFFFFFF0ULL, 0x20) == 0 && user_range_ok(0x1000, 0xFFFFFFFFFFFFFFFFULL) == 0 && user_range_ok(0x8000000000000000ULL, 0x8000000000001000ULL) == 0,
          "an overflowing range was accepted");
    CHECK("an empty range is always fine", user_range_ok(0x1000, 0) == 1 && user_range_ok(KERNEL_PTR, 0) == 1, "should be 1");
    CHECK("a null pointer is not special for the range check", user_range_ok(0, 1) == 1, "should be 1");

    char src[8] = "ABCDEFG", dst[8];
    memset(dst, '.', 8);
    CHECK("copy_from_user copies bytes", copy_from_user(dst, P(src), 7) == 0 && memcmp(dst, "ABCDEFG.", 8) == 0, "dst is \"%.8s\"", dst);
    memset(dst, '.', 8);
    CHECK("copy_from_user refuses a kernel pointer and touches nothing", copy_from_user(dst, KERNEL_PTR, 4) == -EFAULT && memcmp(dst, "........", 8) == 0, "dst is \"%.8s\"", dst);
    char out[8]; memset(out, '.', 8);
    CHECK("copy_to_user writes into the user buffer", copy_to_user(P(out), "xyz", 3) == 0 && memcmp(out, "xyz.....", 8) == 0, "out is \"%.8s\"", out);
    CHECK("copy_to_user refuses a range that wraps", copy_to_user(0xFFFFFFFFFFFFFFF8ULL, "xyz", 16) == -EFAULT, "should be -EFAULT");

    syscall_init();
    reset_os();
    int64_t r = syscall_dispatch(1, 1, P("hello"), 5);
    CHECK("write(1, \"hello\", 5) through the dispatcher returns 5", r == 5, "returned %ld", (long)r);
    CHECK("and the console received exactly those bytes", console_len == 5 && memcmp(console_log, "hello", 5) == 0, "console has %lu bytes", (unsigned long)console_len);
    CHECK("write to fd 2 works as well", syscall_dispatch(1, 2, P("!"), 1) == 1 && console_log[5] == '!', "returned something else");
    CHECK("write to any other fd is -EBADF", syscall_dispatch(1, 3, P("x"), 1) == -EBADF && syscall_dispatch(1, 0, P("x"), 1) == -EBADF, "should be -EBADF");
    CHECK("write of zero bytes returns 0", syscall_dispatch(1, 1, P("x"), 0) == 0, "should be 0");

    reset_os();
    r = syscall_dispatch(1, 1, KERNEL_PTR, 16);
    CHECK("write from a kernel pointer is -EFAULT and prints nothing", r == -EFAULT && console_len == 0, "returned %ld, console has %lu bytes", (long)r, (unsigned long)console_len);
    r = syscall_dispatch(1, 1, USER_TOP - 3, 10);
    CHECK("a buffer that straddles the end of user space is -EFAULT", r == -EFAULT && console_len == 0, "returned %ld, console has %lu bytes", (long)r, (unsigned long)console_len);
    r = syscall_dispatch(1, 1, 0x1000, 0xFFFFFFFFFFFFFFFFULL);
    CHECK("a gigantic length is -EFAULT, not a crash or a huge loop", r == -EFAULT && console_len == 0, "returned %ld", (long)r);
    r = syscall_dispatch(1, 3, KERNEL_PTR, 1);
    CHECK("the file descriptor is checked before the buffer", r == -EBADF, "returned %ld", (long)r);

    static char big[200];
    for (int i = 0; i < 200; i++) big[i] = (char)('a' + i % 26);
    reset_os();
    r = syscall_dispatch(1, 1, P(big), 200);
    CHECK("a 200-byte write is delivered completely and in order", r == 200 && console_len == 200 && memcmp(console_log, big, 200) == 0, "returned %ld, console has %lu bytes", (long)r, (unsigned long)console_len);
    CHECK("it goes through the console in chunks of at most 64 bytes", console_max_chunk <= 64 && console_calls >= 4, "largest chunk %lu bytes in %lu calls", (unsigned long)console_max_chunk, (unsigned long)console_calls);

    reset_os();
    char buf[16]; memset(buf, '.', 16);
    r = syscall_dispatch(0, 0, P(buf), 3);
    CHECK("read returns the first bytes of the pending input", r == 3 && memcmp(buf, "hel", 3) == 0 && buf[3] == '.', "returned %ld, buf \"%.4s\"", (long)r, buf);
    r = syscall_dispatch(0, 0, P(buf), 10);
    CHECK("the next read continues where the last stopped and returns only what is left", r == 2 && memcmp(buf, "lo", 2) == 0, "returned %ld", (long)r);
    CHECK("once the input is used up read returns 0", syscall_dispatch(0, 0, P(buf), 10) == 0, "should be 0");
    CHECK("read from fd 1 is -EBADF", syscall_dispatch(0, 1, P(buf), 1) == -EBADF, "should be -EBADF");
    reset_os();
    CHECK("read into a kernel pointer is -EFAULT and consumes nothing", syscall_dispatch(0, 0, KERNEL_PTR, 4) == -EFAULT && input_pos == 0, "returned something else, input_pos %lu", (unsigned long)input_pos);

    CHECK("getpid returns the current process id", syscall_dispatch(39, 0, 0, 0) == 42, "returned %ld", (long)syscall_dispatch(39, 0, 0, 0));
    reset_os();
    CHECK("exit records the exit code", syscall_dispatch(60, 7, 0, 0) == 0 && exited == 1 && exit_code == 7, "exited %d, code %d", exited, exit_code);

    CHECK("an unused syscall number is -ENOSYS", syscall_dispatch(2, 0, 0, 0) == -ENOSYS && syscall_dispatch(63, 0, 0, 0) == -ENOSYS, "should be -ENOSYS");
    CHECK("numbers beyond the table are -ENOSYS, not a read past the array", syscall_dispatch(64, 0, 0, 0) == -ENOSYS && syscall_dispatch(1000000, 0, 0, 0) == -ENOSYS && syscall_dispatch(0xFFFFFFFFFFFFFFFFULL, 0, 0, 0) == -ENOSYS, "should be -ENOSYS");
}
