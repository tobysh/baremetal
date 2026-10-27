package main

import "fmt"

type Account struct {
	Owner   string
	Balance int
}

// NewAccount returns a new account with the given owner and opening balance.
func NewAccount(owner string, opening int) *Account {
	return &Account{Owner: owner, Balance: opening}
}

// Deposit adds n to the balance (ignoring non-positive n).
func (a *Account) Deposit(n int) {
	if n > 0 {
		a.Balance += n
	}
}

// Withdraw removes n and reports success; it never overdraws.
func (a *Account) Withdraw(n int) bool {
	if n <= 0 || n > a.Balance {
		return false
	}
	a.Balance -= n
	return true
}

// Summary returns "<owner>: <balance>".
func (a Account) Summary() string {
	return fmt.Sprintf("%s: %d", a.Owner, a.Balance)
}
