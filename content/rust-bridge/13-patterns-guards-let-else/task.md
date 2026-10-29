The starter defines `enum Cmd { Move { dx: i32, dy: i32 }, Say(String), Quit }` and `enum Shape { Rect { w: u32, h: u32 }, Circle { r: u32 }, Triangle(u32, u32, u32) }`. Complete:

- `fn grade(score: u32) -> char`: 90-100 gives `'A'`, 80-89 `'B'`, 70-79 `'C'`, 60-69 `'D'`, 0-59 `'F'`, anything above 100 gives `'?'`. Use range patterns.
- `fn parse_cmd(line: &str) -> Option<Cmd>`: `"quit"` is `Quit`; `"say <text>"` is `Say(text)` (the text is everything after the first space and may contain spaces, but must not be empty); `"move <dx> <dy>"` is `Move` with two integers (negative allowed, exactly two numbers). Anything else is `None`. Use `let ... else` at least once.
- `fn classify(p: (i32, i32)) -> &'static str`: `"origin"` for (0,0), `"x-axis"` when only y is 0, `"y-axis"` when only x is 0, `"diagonal"` when `x == y` or `x == -y` (and not the origin), otherwise `"plane"`.
- `fn describe(s: &Shape) -> String`: `Rect` with equal sides is `"square 5"` (the side), otherwise `"rect 3x4"`; `Circle` is `"circle r=2"`; a `Triangle` with three equal sides is `"equilateral"`, with exactly two equal sides `"isosceles"`, with none equal `"scalene"`.
- `fn bucket(n: i64) -> String`: for negative numbers `"negative"`; for 0 `"zero"`; for 1 to 9 the text `"digit N"` (with the number, using `@`); for 10 to 99 `"two digits"`; otherwise `"large"`.
