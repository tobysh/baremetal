---
title: Structs and methods
summary: Group data into structs, attach methods to your own types, and learn when a method needs a pointer receiver.
minutes: 20
difficulty: medium
id: go-foundations/structs-and-methods
mode: harness
---
Go has no classes. You group fields in a **struct** and attach **methods** to the type.

```go
type Account struct {
	Owner   string
	Balance int
}

a := Account{Owner: "Ana", Balance: 100}   // named fields (preferred)
b := Account{"Bo", 50}                     // positional
var zero Account                           // all fields zero: "" and 0
fmt.Println(a.Owner, a.Balance)
```

Fields and methods whose names start with a capital letter are **exported** (visible to other packages); lowercase ones are private to the package.

## Methods

A method is a function with a **receiver** written before its name:

```go
func (a Account) Summary() string {
	return fmt.Sprintf("%s: %d", a.Owner, a.Balance)
}
```

## Value versus pointer receivers

Go passes everything by value, and that includes the receiver. A method with a value receiver `(a Account)` works on a **copy**, so changes are lost. To modify the struct, use a pointer receiver `(a *Account)`:

```go
func (a *Account) Deposit(n int) {
	a.Balance += n          // a.Balance is shorthand for (*a).Balance
}

acct := Account{Owner: "Ana"}
acct.Deposit(25)            // Go takes &acct for you
```

Rules of thumb: use a pointer receiver when the method changes the struct or the struct is large; use the same kind of receiver for all methods of a type.

## Constructors

Go has no constructors; by convention a function called `NewX` builds a value, often returning a pointer:

```go
func NewAccount(owner string) *Account {
	return &Account{Owner: owner}
}
```

`&Account{...}` allocates the struct and gives you its address. Unlike C, returning a pointer to a local is perfectly safe: the compiler moves the value to the heap when needed.
