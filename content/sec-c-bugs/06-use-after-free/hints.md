Use `strlen(src) + 1` bytes for the terminator.
===
Allocate, check for NULL, copy, and return. The caller performs the free.
