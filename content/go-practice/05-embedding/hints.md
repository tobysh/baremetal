`Logf`: `l.lines = append(l.lines, l.Prefix+": "+fmt.Sprintf(format, args...))`.
===
`NewAccount`: `return &Account{Logger: Logger{Prefix: owner}, Balance: balance}`. The embedded field is named `Logger`.
===
`Deposit` and `Withdraw` call the promoted method directly: `a.Logf("deposit %d", n)`. In `Withdraw`, test `n > a.Balance` first.
===
`NewPremium`: `return &Premium{Account: NewAccount(owner, balance), Fee: fee}`. `Premium.Withdraw` is `return p.Account.Withdraw(n + p.Fee)`. Writing `p.Withdraw` inside it would call itself forever.
===
`Summary`: `strings.Join(a.Lines(), "; ")`.
