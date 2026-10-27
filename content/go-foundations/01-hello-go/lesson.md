---
title: Hello, Go
summary: Meet Go, a small compiled language with fast builds. Write package main, declare variables with var and :=, and print with fmt.
minutes: 12
difficulty: easy
id: go-foundations/hello-go
mode: stdout
---
**Go** (often called Golang) was created at Google to make servers and command-line tools easy to write, quick to compile and safe to run. It is compiled to a single native binary like C, but it has a garbage collector, so you never call `free`, and it has concurrency built into the language, which you will meet near the end of this course.

## The smallest Go program

```go
package main

import "fmt"

func main() {
	fmt.Println("Hi!")
}
```

- Every file starts with a `package`. A program that you can run is `package main`.
- `import "fmt"` brings in the `fmt` package, Go's formatted input and output. Go refuses to compile if you import something and do not use it.
- `func main()` is where execution starts. There are no semicolons, and the opening `{` must be on the same line as `func`.
- `fmt.Println` prints its arguments separated by spaces and ends with a newline. Exported names such as `Println` always start with a capital letter.

## Variables and types

```go
var language string = "Go"   // full form
year := 2009                 // short form: type is inferred (int)
const pi = 3.14159           // a constant
var ready bool               // no value given: the zero value, false
```

`:=` declares and assigns in one go, but only works inside functions. Go is **statically typed**: `year` is an `int` forever, and you cannot mix an `int` with a `float64` without converting: `float64(year) * 1.5`. Every type has a **zero value** (`0`, `""`, `false`, `nil`), so variables are never uninitialised garbage like in C. And Go will not compile a local variable you never use.

## Printing with formats

`fmt.Printf` takes a format string. The common verbs are `%d` (integer), `%s` (string), `%v` (any value, the default format), `%t` (boolean), `%.2f` (float with two decimals) and `%q` (quoted string). Unlike `Println`, `Printf` does not add a newline, so end with `\n`.

```go
fmt.Printf("%s was released in %d\n", language, year)
```
