---
title: Interfaces
summary: Define behaviour with interfaces that types satisfy implicitly, then use type switches to recover the concrete type.
minutes: 20
difficulty: medium
id: go-foundations/interfaces
mode: harness
---
An **interface** is a set of method signatures. In Go a type satisfies an interface **implicitly**: if it has the methods, it implements the interface, with no `implements` keyword anywhere.

```go
type Shape interface {
	Area() float64
}

type Rect struct{ W, H float64 }
type Circle struct{ R float64 }

func (r Rect) Area() float64   { return r.W * r.H }
func (c Circle) Area() float64 { return math.Pi * c.R * c.R }

func Total(shapes []Shape) float64 {
	sum := 0.0
	for _, s := range shapes {
		sum += s.Area()       // dynamic dispatch: calls Rect's or Circle's method
	}
	return sum
}

Total([]Shape{Rect{3, 4}, Circle{1}})
```

`Total` knows nothing about rectangles or circles. That is how Go code stays decoupled: functions take small interfaces (`io.Reader`, `fmt.Stringer`, `error`), and any type can plug in.

## Type switches and assertions

An interface value holds a concrete value and its type. You can get it back:

```go
func describe(s Shape) string {
	switch v := s.(type) {
	case Rect:
		return fmt.Sprintf("rect %vx%v", v.W, v.H)
	case Circle:
		return fmt.Sprintf("circle r=%v", v.R)
	default:
		return "unknown"
	}
}

r, ok := s.(Rect)   // single assertion: ok is false if s is not a Rect
```

## The empty interface

`any` (an alias for `interface{}`) is satisfied by every type. It is flexible but throws away type safety, so prefer a real interface when you can.

## nil gotcha

An interface value is `nil` only when it has neither a type nor a value. A nil `*Rect` stored in a `Shape` is a non-nil interface, a classic source of bugs.
