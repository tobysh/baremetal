---
title: Value vs pointer receivers
summary: Choose between func (t T) and func (t *T) methods, and learn how method sets decide which types satisfy an interface.
minutes: 25
difficulty: medium
tags: methods, pointers, interfaces
id: go-practice/value-vs-pointer-receivers
mode: harness
---
A method's **receiver** is just a parameter, and Go passes parameters by value. So there are two kinds of method:

```go
func (c Counter) Value() int { return c.n }   // value receiver: works on a COPY
func (c *Counter) Inc()      { c.n++ }        // pointer receiver: works on the original
```

If `Inc` had a value receiver, it would increment a copy and the caller would see nothing change. This is the single most common receiver bug.

## Choosing

Use a **pointer receiver** when:

- the method must modify the receiver;
- the struct is large and copying it would be wasteful;
- the type contains something that must not be copied (a `sync.Mutex`).

Use a **value receiver** for small, immutable values such as a point, a time or a rectangle's area. Whichever you choose, **be consistent**: if one method of a type needs a pointer receiver, give all of them pointer receivers.

Go helps with the call: if `c` is an addressable variable, `c.Inc()` is rewritten as `(&c).Inc()`. That does not work for things that are not addressable, such as map elements or the return value of a function.

## Method sets

The **method set** of a type decides what interfaces it satisfies:

| type | its method set contains |
| --- | --- |
| `T` | methods with receiver `T` |
| `*T` | methods with receiver `T` **and** `*T` |

```go
type Scaler interface{ Scale(f float64) }

func (r *Rect) Scale(f float64) { ... }

var a Scaler = &Rect{2, 3}  // fine
var b Scaler = Rect{2, 3}   // compile error: Rect does not implement Scaler
                            // (method Scale has pointer receiver)
```

A value stored in an interface is not addressable, so Go refuses to guess. Store the pointer instead.

## The range trap

`for _, c := range counters` gives you a **copy** of each element. Calling a pointer method on `c` changes the copy. Index into the slice instead: `counters[i].Inc()`.
