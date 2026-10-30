Finish the `Account` type:

- `NewAccount(owner string, opening int) *Account` returns a new account with that owner and opening balance.
- `(*Account) Deposit(n int)` adds `n` to the balance. Ignore non-positive amounts.
- `(*Account) Withdraw(n int) bool` subtracts `n` and returns `true`, but if `n` is not positive or exceeds the balance it changes nothing and returns `false`.
- `(Account) Summary() string` returns `"<owner>: <balance>"`, for example `"Ana: 100"`.
