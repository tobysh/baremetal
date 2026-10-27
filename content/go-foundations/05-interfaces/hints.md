Rect: `Area` is `W * H`, `Perimeter` is `2 * (W + H)`. Circle: `math.Pi * R * R` and `2 * math.Pi * R` (add `import "math"`).
===
`Largest`: return `nil` for an empty slice, otherwise remember the best shape so far and compare `s.Area()` with `best.Area()` while looping.
===
`Describe` is a type switch: `switch v := s.(type) { case Rect: ... case Circle: ... default: ... }`, with `fmt.Sprintf("rect %vx%v", v.W, v.H)`.
