---
title: Embedding and composition
summary: Go has no inheritance. Embed one type in another to promote its fields and methods, override them by shadowing, and satisfy interfaces by composition.
minutes: 25
difficulty: medium
tags: structs, composition, interfaces
id: go-practice/embedding
mode: harness
---
Go does not have classes or inheritance. It builds bigger types out of smaller ones by **embedding**: a field with a type and no name.

```go
type Logger struct {
	Prefix string
	lines  []string
}

func (l *Logger) Logf(format string, args ...any) {
	l.lines = append(l.lines, l.Prefix+": "+fmt.Sprintf(format, args...))
}

type Account struct {
	Logger          // embedded
	Balance int
}
```

## Promotion

The fields and methods of the embedded type are **promoted**: you can use them as if they belonged to the outer type.

```go
a := Account{Logger: Logger{Prefix: "alice"}}
a.Logf("opened")        // same as a.Logger.Logf("opened")
fmt.Println(a.Prefix)   // same as a.Logger.Prefix
```

The embedded value is still a normal field named after its type, so `a.Logger` works too, and it is how you set it in a struct literal.

Promotion also feeds method sets. Because `*Account` gets `Logf` from `Logger`, it satisfies any interface that asks for `Logf`, with no forwarding code. This is composition: `Account` **has a** `Logger`, it is not a kind of one.

## Shadowing is not overriding

An outer type may define a method with the same name as a promoted one. The outer one wins when you call it on the outer type, but the inner type knows nothing about it:

```go
type Premium struct {
	*Account       // embedding a pointer works too
	Fee int
}

func (p *Premium) Withdraw(n int) bool {
	return p.Account.Withdraw(n + p.Fee) // call the inner one explicitly
}
```

There are no virtual methods: if `Account` has a method that calls `a.Withdraw(...)` internally, it still calls `Account.Withdraw`, never the `Premium` version.

Two more things to know. Embedding a pointer (`*Account`) shares one account between wrappers but panics if it is `nil`. And if two embedded types promote the same name at the same depth, using that name is a compile error until you pick one explicitly.
