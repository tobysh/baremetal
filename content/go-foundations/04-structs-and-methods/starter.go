package main

type Account struct {
	Owner   string
	Balance int
}

// NewAccount returns a new account with the given owner and opening balance.
func NewAccount(owner string, opening int) *Account {
	return nil
}

// Deposit adds n to the balance (ignoring non-positive n).
func (a *Account) Deposit(n int) {
}

// Withdraw removes n and reports success; it never overdraws.
func (a *Account) Withdraw(n int) bool {
	return false
}

// Summary returns "<owner>: <balance>".
func (a Account) Summary() string {
	return ""
}
