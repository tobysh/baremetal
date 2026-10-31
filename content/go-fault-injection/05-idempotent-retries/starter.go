package main

import (
	"errors"
	"fmt"
)

var (
	ErrTimeout      = errors.New("timeout")
	ErrInsufficient = errors.New("insufficient funds")
	ErrConflict     = errors.New("id reused with different amount")
	ErrInvalid      = errors.New("invalid amount")
	ErrGaveUp       = errors.New("gave up, outcome unknown")
)

type Ledger struct {
	balance int
	seen    map[string]int
}

func NewLedger(balance int) *Ledger { return &Ledger{balance: balance, seen: map[string]int{}} }

func (l *Ledger) Balance() int { return l.balance }

// Debit currently applies every request, even repeats.
func (l *Ledger) Debit(id string, amount int) error {
	if amount > l.balance {
		return ErrInsufficient
	}
	l.balance -= amount
	return nil
}

// Pay currently makes a single attempt.
func Pay(call func(id string, amount int) error, id string, amount, maxAttempts int) error {
	return call(id, amount)
}

func main() {
	l := NewLedger(100)
	lose := true
	call := func(id string, amt int) error {
		err := l.Debit(id, amt)
		if lose {
			lose = false
			return ErrTimeout // reply lost after the work was done
		}
		return err
	}
	err := Pay(call, "order-1", 30, 3)
	fmt.Println("err:", err, "balance:", l.Balance())
}
