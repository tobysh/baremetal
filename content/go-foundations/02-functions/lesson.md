---
title: Functions and multiple returns
summary: Write Go functions with typed parameters, multiple return values, named results and variadic arguments.
minutes: 15
difficulty: easy
tags: functions
id: go-foundations/functions
mode: harness
---
In Go a function lists the **type after the name**, for parameters and for results:

```go
func add(a int, b int) int { return a + b }
func add2(a, b int) int    { return a + b }   // same type: share it
```

## Multiple return values

A function can return several values. This is how Go reports errors (you will see it in a later lesson) and how you avoid output pointers like C's `int *out`:

```go
func swap(a, b string) (string, string) {
	return b, a
}

x, y := swap("left", "right")   // x == "right", y == "left"
_, y2 := swap("a", "b")         // _ discards a value you don't need
```

You must receive every result (or discard it with `_`). You can also **name** the results; they start at their zero value and a bare `return` returns them:

```go
func rect(w, h int) (area, perimeter int) {
	area = w * h
	perimeter = 2 * (w + h)
	return
}
```

## Variadic functions

A final parameter written `...T` accepts any number of arguments and arrives as a slice `[]T`. To pass an existing slice, spread it with `...`:

```go
func sum(nums ...int) int {
	total := 0
	for _, n := range nums {
		total += n
	}
	return total
}

sum()            // 0
sum(1, 2, 3)     // 6
xs := []int{4, 5}
sum(xs...)       // 9
```

## Loops

Go has one loop keyword, `for`:

```go
for i := 0; i < 3; i++ { ... }       // classic
for cond { ... }                      // like while
for { ... }                           // forever, leave with break
for i, v := range slice { ... }       // index and value
```
