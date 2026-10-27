---
title: Slices and maps
summary: Work with Go's two workhorse collections: growable slices with append and slicing, and hash maps with the comma-ok idiom.
minutes: 20
difficulty: medium
id: go-foundations/slices-and-maps
mode: harness
---
## Arrays and slices

An array has a fixed length that is part of its type (`[3]int`). You will almost always use a **slice** instead: a small header holding a pointer to an array, a length and a capacity.

```go
nums := []int{10, 20, 30}      // slice literal
nums = append(nums, 40)        // append returns the (possibly new) slice: always assign it back
fmt.Println(len(nums), cap(nums))

part := nums[1:3]              // [20 30], shares memory with nums!
empty := make([]int, 0, 8)     // length 0, capacity 8
var nothing []int              // nil slice: len 0, and append works on it
```

`s[a:b]` includes `a` and excludes `b`. Because a sub-slice shares memory with the original, writing `part[0] = 99` also changes `nums[1]`. When you need an independent copy, use `copy(dst, src)` or `append([]int(nil), src...)`.

## Maps

A map is a hash table from keys to values:

```go
ages := map[string]int{"ana": 31, "bo": 27}
ages["cy"] = 19
delete(ages, "bo")

age, ok := ages["dan"]      // comma-ok: ok is false when the key is missing
fmt.Println(age, ok)        // 0 false  (missing keys give the zero value)

for name, age := range ages { ... }   // iteration order is random!
```

A `map` declared with `var m map[string]int` is **nil**: you can read it but writing panics. Create maps with `make` or a literal. Counting is easy because a missing key reads as `0`:

```go
counts := make(map[string]int)
for _, w := range words {
	counts[w]++
}
```

## Strings

`strings.Fields(s)` splits on runs of whitespace, `strings.ToLower(s)` lower-cases. Remember to `import "strings"` first.
