---
title: Defer, panic and recover
summary: Run cleanup with defer, understand when Go panics, and use recover to turn a panic back into an ordinary error at a boundary you control.
minutes: 22
difficulty: medium
tags: defer, panic, errors
id: go-foundations/defer-panic-recover
mode: harness
---
## defer

`defer` schedules a function call to run when the surrounding function **returns**, however it returns: normally, by an early `return`, or by panicking. It is Go's tool for cleanup that must not be forgotten.

```go
f, err := os.Open(path)
if err != nil { return err }
defer f.Close()          // runs on every path out of this function
```

Three rules to remember:

1. Deferred calls run in **last-in, first-out** order.
2. The **arguments** of a deferred call are evaluated immediately, when the `defer` statement runs. The call itself happens later.
3. A deferred *closure* can read and change the function's **named results**, because it runs after the `return` has set them.

```go
func double(n int) (result int) {
	defer func() { result *= 2 }()   // runs after return n + 1
	return n + 1
}                                    // double(3) == 8
```

## panic

`panic(v)` stops the normal flow: the function's deferred calls run, then the caller's, and so on up the stack. If nothing stops it, the program crashes with a stack trace. The runtime panics for you on bugs: a nil pointer dereference, an index out of range, integer division by zero, a send on a closed channel.

Go code does **not** use panics as everyday error handling. You return an `error`. Panics are for "this should be impossible" and for programmer mistakes.

## recover

`recover()` stops a panic and returns the value given to `panic`. It only works **inside a deferred function**, and returns `nil` when nothing is panicking.

```go
func safe() (err error) {
	defer func() {
		if r := recover(); r != nil {
			err = fmt.Errorf("recovered: %v", r)
		}
	}()
	riskyThing()
	return nil
}
```

Use it at a boundary, for example around a request handler or a plug-in call, so one failure does not take everything down. Do not use it to hide bugs. The value `r` is an `any`: it could be a string, an `error`, or a runtime error, so check its type with a type switch before you use it.
