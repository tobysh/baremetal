Fix `add_item(cart *c, int qty, int price)` and complete `diagnosis()`.

- If the cart has room, store `qty` in the next slot, increase `used`, add `qty * price` to `total`, and return `0`.
- If the cart is full (`used == CAP`), return `-1` and change nothing: not `qty`, not `used`, not `total`, and no byte outside the struct.
- `diagnosis()` returns the `diagnosis_code` that names the root cause of the corrupted `used` counter in call 5 of the evidence table. Do not change the enum, the struct or `CAP`.

The two faults are separate: one shows from the first call, one only when the cart is full. Fixing only the visible one leaves the table wrong. Many different sequences are tried, including carts that are filled and then pushed many more times.
