---
title: Generics
summary: Write functions and types that work for many element types with type parameters and constraints.
minutes: 30
difficulty: medium
tags: generics, functions
id: go-practice/generics
mode: harness
---
Before generics, a `Filter` for `[]int` and a `Filter` for `[]string` were two copies of the same code. A **type parameter** lets you write it once.

```go
func Map[T, U any](xs []T, f func(T) U) []U {
	out := make([]U, 0, len(xs))
	for _, x := range xs {
		out = append(out, f(x))
	}
	return out
}

names := Map([]int{1, 2, 3}, strconv.Itoa) // []string{"1", "2", "3"}
```

`[T, U any]` declares two type parameters. `any` is the **constraint**: it says which types are allowed (here, all of them). Go works out `T` and `U` from the arguments, so you rarely write them at the call site.

## Constraints

Some operations need more than `any`. You cannot compare with `<` unless the type supports it, so ask for that:

```go
import "cmp"

func Max[T cmp.Ordered](xs []T) (T, bool) {
	var zero T
	if len(xs) == 0 {
		return zero, false
	}
	best := xs[0]
	for _, x := range xs[1:] {
		if x > best {
			best = x
		}
	}
	return best, true
}
```

`cmp.Ordered` covers every integer, float and string type. `var zero T` gives the zero value of whatever `T` turned out to be, which is how you return "nothing" from a generic function.

## Generic types

A type can have parameters too, and its methods repeat them on the receiver:

```go
type Stack[T any] struct {
	items []T
}

func (s *Stack[T]) Push(x T) { s.items = append(s.items, x) }
```

Use generics when the *same algorithm* applies to many types. If you only ever call the function with one type, or the behaviour differs per type, a plain function or an interface is clearer.
