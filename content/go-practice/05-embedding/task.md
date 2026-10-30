`Logger`, `Account` (which embeds `Logger`) and `Premium` (which embeds `*Account`) are already declared. Write the behaviour.

- `(l *Logger) Logf(format string, args ...any)` appends `l.Prefix + ": " + fmt.Sprintf(format, args...)` to the logger's lines. (`Lines()` already returns them.)
- `NewAccount(owner string, balance int) *Account` makes an account whose logger prefix is `owner`.
- `(a *Account) Deposit(n int)` adds `n` to the balance and logs `"deposit N"`.
- `(a *Account) Withdraw(n int) bool` returns `false` and logs `"refused N"` if `n` is more than the balance. Otherwise it subtracts `n`, logs `"withdraw N"` and returns `true`.
- `NewPremium(owner string, balance, fee int) *Premium` wraps a new account and a fee.
- `(p *Premium) Withdraw(n int) bool` withdraws `n` **plus** the fee, by calling the inner account's `Withdraw`. Its log lines therefore show the total amount.
- `Summary(a Auditable) string` joins `a.Lines()` with `"; "` (the empty string for no lines). `Auditable` is the interface `Lines() []string`; both `*Account` and `*Premium` satisfy it through promotion, so you write no extra methods.

Example: an account `"ann"` with balance 100 after `Deposit(50)` and `Withdraw(500)` has the lines `"ann: deposit 50"` and `"ann: refused 500"`.
