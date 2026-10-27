Keep the two most recent values in the struct: `Fib { a: u64, b: u64 }`, starting at `a = 0, b = 1`. In `next`, remember `a`, then move forward: `let next = self.a.checked_add(self.b)?; let out = self.a; self.a = self.b; self.b = next; Some(out)`.
===
For `Countdown`, if `self.n == 0` return `None`. Otherwise copy `self.n`, subtract one from `self.n`, and return `Some(copy)`.
===
`fib().take_while(|&n| n < limit).filter(|n| n % 2 == 0).sum()`. `take_while` stops the endless-looking sequence, and `sum` needs the type written somewhere: the function's return type is enough.
