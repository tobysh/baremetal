.intel_syntax noprefix
.text
.globl reverse_array

# void reverse_array(long *a, size_t n)
# a in rdi, n in rsi. Reverse the array in place; nothing to return.
reverse_array:
    ret                 # TODO: replace with the real work

.section .note.GNU-stack,"",@progbits
