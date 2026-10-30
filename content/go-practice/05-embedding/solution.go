package main

import (
	"fmt"
	"strings"
)

// Logger records prefixed lines.
type Logger struct {
	Prefix string
	lines  []string
}

// Logf appends Prefix + ": " + the formatted message.
func (l *Logger) Logf(format string, args ...any) {
	l.lines = append(l.lines, l.Prefix+": "+fmt.Sprintf(format, args...))
}

func (l *Logger) Lines() []string { return l.lines }

// Account has a Logger. Its methods are promoted to *Account.
type Account struct {
	Logger
	Balance int
}

func NewAccount(owner string, balance int) *Account {
	return &Account{Logger: Logger{Prefix: owner}, Balance: balance}
}

func (a *Account) Deposit(n int) {
	a.Balance += n
	a.Logf("deposit %d", n)
}

func (a *Account) Withdraw(n int) bool {
	if n > a.Balance {
		a.Logf("refused %d", n)
		return false
	}
	a.Balance -= n
	a.Logf("withdraw %d", n)
	return true
}

// Premium wraps an account and charges a fee on every withdrawal.
type Premium struct {
	*Account
	Fee int
}

func NewPremium(owner string, balance, fee int) *Premium {
	return &Premium{Account: NewAccount(owner, balance), Fee: fee}
}

// Withdraw takes n plus the fee.
func (p *Premium) Withdraw(n int) bool {
	return p.Account.Withdraw(n + p.Fee)
}

// Auditable is anything that can list its log lines.
type Auditable interface{ Lines() []string }

// Summary joins the lines with "; ".
func Summary(a Auditable) string {
	return strings.Join(a.Lines(), "; ")
}
