The caller supplies the address of its pointer, so change `*owner`.
===
Check owner, call `free(*owner)`, and assign `*owner = NULL`.
