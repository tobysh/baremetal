---
title: Pointers
summary: Understand Go pointers: & and *, pass by value, nil, new, and building a linked list without pointer arithmetic.
minutes: 18
difficulty: medium
tags: pointers
id: go-foundations/pointers
mode: harness
---
If you have used C pointers, Go's are the same idea with the dangerous parts removed: **no pointer arithmetic**, automatic memory management, and a clear `nil`.

```go
x := 10
p := &x          // p is a *int holding x's address
*p = 20          // write through the pointer: x is now 20
fmt.Println(*p)  // read through it: 20
```

## Everything is passed by value

When you call a function, Go copies each argument. Changing the copy never affects the caller, unless the copy is a pointer to the caller's data:

```go
func incrementValue(n int) { n++ }       // modifies a copy
func incrementPointer(n *int) { *n++ }   // modifies the caller's variable

a := 1
incrementValue(a)    // a is still 1
incrementPointer(&a) // a is now 2
```

(Slices, maps and channels are small descriptors that already point at shared data, which is why a function can change a slice's elements.)

## nil and new

The zero value of a pointer is `nil`. Dereferencing nil panics with `invalid memory address or nil pointer dereference`, Go's version of a segfault, so check before you use a pointer that might be nil. `new(T)` allocates a zeroed `T` and returns `*T`; `&T{...}` does the same with initial values.

Escape analysis decides whether a value lives on the stack or heap, so it is safe to return `&local`.

## A linked list

Pointers shine in recursive structures:

```go
type Node struct {
	Value int
	Next  *Node
}

head := &Node{1, &Node{2, nil}}
for n := head; n != nil; n = n.Next {
	fmt.Println(n.Value)
}
```

`n.Next` works on a pointer without writing `(*n).Next`: Go dereferences for you.
