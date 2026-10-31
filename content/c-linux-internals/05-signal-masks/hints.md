Start with `sig_mask` and `sig_set_disp`; they have no hidden interactions. Then `sig_send`, then `sig_deliver`.
===
Delivery picks `pending & ~blocked`. Walk upward from 1 to find the lowest set bit. For a stopped process, mask that value down to just `SIGKILL` first.
===
Order inside `sig_send`: `SIGCONT` side effects happen first and unconditionally. Only after that decide whether the signal is discarded (ignored and not blocked) or recorded.
