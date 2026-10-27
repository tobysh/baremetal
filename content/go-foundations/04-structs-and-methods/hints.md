`NewAccount` can be one line: `return &Account{Owner: owner, Balance: opening}`.
===
`Deposit` and `Withdraw` must use a pointer receiver `(a *Account)` or the change happens on a copy and is lost. Check the amount (`n <= 0`) before touching the balance.
===
`Summary` can use `fmt.Sprintf("%s: %d", a.Owner, a.Balance)`. Remember `import "fmt"` at the top of the file.
