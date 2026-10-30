---
title: Control flow
summary: Go has one loop keyword, for, and a switch that does not fall through. Use for, range, break, continue, switch and early return.
minutes: 15
difficulty: easy
tags: control-flow, loops
id: go-foundations/control-flow
mode: harness
---
Go keeps control flow small. There is **one** loop keyword (`for`), one multi-way branch (`switch`), and `if` without parentheses.

## for, in three shapes

```go
for i := 0; i < 5; i++ { ... }   // the classic three-part loop
for n > 1 { ... }                // only a condition: Go's "while"
for { ... }                      // no condition: loops until break or return
```

`range` walks a slice, array, string, map or channel. It gives an index and a value, and `_` discards the one you do not need:

```go
for i, v := range []string{"a", "b"} { fmt.Println(i, v) }
for _, v := range xs { total += v }
for i := range 3 { ... }          // Go 1.22+: i is 0, 1, 2
```

`break` leaves the innermost loop. `continue` jumps to its next iteration. A `return` inside a loop leaves the whole function, which is often the clearest way to stop early.

## switch

```go
switch day {
case "sat", "sun":      // several values in one case
	return "weekend"
case "mon", "tue":
	return "weekday"
default:
	return "unknown"
}
```

Unlike C, a case **does not fall through** to the next one, so there is no `break` to forget. A `switch` with no value checks each case as a condition, which reads better than a chain of `else if`:

```go
switch {
case score >= 90:
	return "A"
case score >= 80:
	return "B"
}
```

## if with a short statement

`if v, ok := m[key]; ok { ... }` declares `v` and `ok` just for the `if` and its `else` branches.
