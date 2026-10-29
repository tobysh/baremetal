Each operator impl follows the same pattern: `impl Add for Cents { type Output = Cents; fn add(self, o: Cents) -> Cents { Cents(self.0 + o.0) } }`. `Mul<i64>` takes an `i64` on the right, and `Neg` has no right-hand side.
===
`AddAssign` is `fn add_assign(&mut self, o: Cents) { self.0 += o.0; }`. `Sum` can use `iter.fold(Cents(0), |a, b| a + b)`.
===
For `Display`, take the sign and absolute value separately: `let sign = if self.0 < 0 { "-" } else { "" }; let a = self.0.abs();` then `write!(f, "{sign}${}.{:02}", a / 100, a % 100)`.
