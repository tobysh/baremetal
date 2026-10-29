Start `kbd_feed` with the prefix: `if (sc == 0xE0) { s->e0 = 1; return 0; }`. Then `if (s->e0) { s->e0 = 0; ...handle the four arrows and right Ctrl, return 0 otherwise }` so the flag is always consumed by exactly one following byte.
===
Split the byte: `int released = sc & 0x80; uint8_t code = sc & 0x7F;`. For both kinds of event, update modifiers in one place: `if (code == 0x2A) { set or clear bit 0 of s->shift }`, `0x36` bit 1, `0x1D` the ctrl flag. A release then returns 0 right after that.
===
Caps Lock toggles only on a make code: `if (code == 0x3A) { if (!released) s->caps ^= 1; return 0; }`.
===
Look up the character: `char base = scan_normal[code];` (after checking `code < 58`). `int is_letter = base >= 'a' && base <= 'z';`. Use shifted if `is_letter ? ((s->shift != 0) ^ s->caps) : (s->shift != 0)`. For Ctrl, use `base - 'a' + 1` on a letter, ignoring shift and caps.
===
`kbd_poll`: `uint8_t st = inb(0x64); if (!(st & 1)) return -1; uint8_t b = inb(0x60); if (st & 0x20) return -1; return b;`. Read the data port even for a mouse byte, or the buffer stays full forever.
