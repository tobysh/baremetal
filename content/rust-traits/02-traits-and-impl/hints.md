Write `impl Shape for Square { ... }` with the three required methods: `name`, `area` and `perimeter`. Each takes `&self` and uses `self.side`.
===
`fn name(&self) -> String { String::from("square") }`, `fn area(&self) -> u32 { self.side * self.side }`. Do the same for `Rectangle` with `self.width` and `self.height`.
===
For `total_area`, loop and add: `let mut sum = 0; for s in shapes { sum += s.area(); } sum`. An iterator version is `shapes.iter().map(|s| s.area()).sum()`.
