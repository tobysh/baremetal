.intel_syntax noprefix
.text
.globl format_pair
.globl joined_length

# int format_pair(char *buf, size_t n, long a, long b)
# Write "<a>,<b>" with snprintf(buf, n, "%ld,%ld", a, b) and return its result.
format_pair:
    mov eax, 0          # TODO: replace with the real work
    ret

# size_t joined_length(const char *a, const char *b)
# Return strlen(a) + strlen(b).
joined_length:
    mov rax, 0          # TODO: replace with the real work
    ret

.section .note.GNU-stack,"",@progbits
