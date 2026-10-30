---
title: Slice internals
summary: A slice is a window onto an array. Learn len versus cap, how append may or may not copy, and how to avoid slices that quietly share memory.
minutes: 22
difficulty: medium
tags: slices, memory
id: go-foundations/slice-internals
mode: harness
---
A slice value is a tiny three-field struct: a **pointer** to an element of an underlying array, a **length** (`len`) and a **capacity** (`cap`, how far it could grow before the array runs out). Copying a slice copies those three fields, not the elements.

```go
a := []int{1, 2, 3, 4, 5}
b := a[1:3]                 // b is [2 3], len 2, cap 4
b[0] = 99                   // a is now [1 99 3 4 5]: same array
```

`s[low:high]` shares memory with `s`, and its capacity runs from `low` to the end of the array.

## append

`append(s, v)` has two outcomes:

- If `len < cap`, the value is written into the **existing array** and the result still shares memory with `s`.
- If the array is full, Go allocates a bigger one, copies the elements and returns a slice on the **new** array.

```go
base := make([]int, 3, 10)
x := append(base, 1)
y := append(base, 2)
fmt.Println(x[3], y[3]) // 2 2: both wrote slot 3 of the same array!
```

You cannot tell from the call which outcome you will get, so always write `s = append(s, v)` and never keep using the old slice by accident.

## Controlling sharing

- **Copy** to get an independent slice: `out := make([]int, len(s)); copy(out, s)`.
- **Full slice expression** `s[low:high:max]` limits the capacity to `max - low`. With `max == high`, any `append` has to reallocate, so it cannot overwrite the elements after the window:

```go
w := a[1:3:3]               // len 2, cap 2
w = append(w, 0)            // reallocates, a is untouched
```

- Deleting while keeping order: `s = append(s[:i], s[i+1:]...)` shifts the tail down in the same array.

A small slice can also keep a huge array alive. If you keep only a few elements of a large slice for a long time, copy them out so the rest can be freed.
