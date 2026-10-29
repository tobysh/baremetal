The `KbdState` struct, the `KEY_*` constants and the two tables `scan_normal` and `scan_shifted` are in the starter. Implement:

- `int kbd_poll(void)` returns the next byte from the controller if the status port (`0x64`) says one is waiting and it came from the keyboard, otherwise `-1`. If bit 5 of the status says the byte came from the mouse, read and discard it, and return `-1`.
- `int kbd_feed(KbdState *s, uint8_t sc)` consumes one scancode and returns what it produced, following the rules in the lesson:
  - `0xE0` sets `s->e0` and returns 0.
  - After `0xE0`: make codes `0x48`, `0x50`, `0x4B`, `0x4D` return `KEY_UP`, `KEY_DOWN`, `KEY_LEFT`, `KEY_RIGHT`; `0x1D` (right Ctrl) sets `s->ctrl` and `0x9D` clears it; everything else returns 0. The flag is cleared after one byte.
  - Break codes (bit 7 set) release Shift (`0x2A`, `0x36`, each its own bit of `s->shift`) and Ctrl (`0x1D`) and return 0.
  - Make codes: Shift sets its bit, Ctrl sets `s->ctrl`, Caps Lock (`0x3A`) toggles `s->caps`, all returning 0. Codes above `0x39` return 0. Otherwise return the character from `scan_shifted` if the shift state says so (see the lesson), else from `scan_normal`. With `s->ctrl` set, a letter returns its control code (1 to 26).
