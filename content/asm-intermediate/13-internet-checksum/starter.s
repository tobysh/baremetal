.intel_syntax noprefix
.text
.globl inet_checksum

# uint16_t inet_checksum(const void *data, size_t len)
inet_checksum:
    mov eax, 0          # TODO
    ret

.section .note.GNU-stack,"",@progbits
