---
title: Sorting and Closures
summary: Sort slices with a comparison function, and use closures that remember state between calls.
minutes: 30
difficulty: medium
tags: sorting, closures
id: go-practice/sorting-and-closures
mode: harness
---
## Sorting with a comparison function

`sort.Slice` sorts any slice. You give it a function that says whether element `i` belongs before element `j`:

```go
people := []Person{{"Bo", 31}, {"Al", 31}, {"Cy", 25}}

sort.Slice(people, func(i, j int) bool {
	return people[i].Age < people[j].Age
})
```

That function is a **closure**: it uses the `people` variable from the code around it. `sort.Slice` is not guaranteed to keep equal elements in their original order. When ties matter, either use `sort.SliceStable` or break the tie yourself:

```go
sort.Slice(people, func(i, j int) bool {
	if people[i].Age != people[j].Age {
		return people[i].Age < people[j].Age
	}
	return people[i].Name < people[j].Name // tie-break
})
```

A comparison function must be consistent: if it says `a` before `b`, it must not also say `b` before `a`. Use `<`, never `<=`.

## Closures that remember

A closure keeps the variables it uses alive, even after the function that created it has returned:

```go
func counter() func() int {
	n := 0
	return func() int {
		n++
		return n
	}
}

next := counter()
next() // 1
next() // 2
other := counter() // its own n
```

Each call to `counter` makes a fresh `n`, so each returned function counts independently. This is how you get a little private state without defining a type.

## Memoising

A pure function always gives the same answer for the same input, so you can remember the answers in a map held by a closure and skip repeated work. That is exactly what `Memoize` below does.
