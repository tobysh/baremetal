Look at the `total` column on its own. It is wrong at call 1, when nothing has overflowed. What is added to `total`?
===
Look at `used` at call 5: 8 is the quantity 7 plus one. Which array slot is directly followed by `used`, and which index of `qty` would write there?
===
Check `c->used < CAP` before touching anything and return `-1` if it fails. Add `qty * price`, not `price`. The root cause of the corruption is the missing capacity check, not the cost formula.
