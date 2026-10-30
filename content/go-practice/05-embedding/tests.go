package main

import (
	"reflect"
	"testing"
)

var _ Auditable = &Account{}
var _ Auditable = &Premium{}

func TestLogger_logf_formats_with_prefix(t *testing.T) {
	l := &Logger{Prefix: "svc"}
	l.Logf("started")
	l.Logf("port %d on %s", 8080, "lo")
	want := []string{"svc: started", "svc: port 8080 on lo"}
	if !reflect.DeepEqual(l.Lines(), want) {
		t.Errorf("lines = %q; want %q", l.Lines(), want)
	}
}

func TestNewAccount_sets_balance_and_prefix(t *testing.T) {
	a := NewAccount("ann", 100)
	if a.Balance != 100 || a.Prefix != "ann" {
		t.Errorf("NewAccount(\"ann\", 100) has balance %d and prefix %q; want 100 and \"ann\"", a.Balance, a.Prefix)
	}
}

func TestAccount_deposit_and_withdraw(t *testing.T) {
	a := NewAccount("ann", 100)
	a.Deposit(50)
	if ok := a.Withdraw(500); ok {
		t.Errorf("Withdraw(500) from 150 returned true; want false")
	}
	if ok := a.Withdraw(150); !ok {
		t.Errorf("Withdraw(150) from 150 returned false; want true (exactly the balance is allowed)")
	}
	if a.Balance != 0 {
		t.Errorf("balance = %d; want 0", a.Balance)
	}
	want := []string{"ann: deposit 50", "ann: refused 500", "ann: withdraw 150"}
	if !reflect.DeepEqual(a.Lines(), want) {
		t.Errorf("lines = %q; want %q", a.Lines(), want)
	}
}

func TestAccount_refused_withdrawal_keeps_balance(t *testing.T) {
	a := NewAccount("bo", 10)
	a.Withdraw(11)
	if a.Balance != 10 {
		t.Errorf("a refused withdrawal changed the balance to %d", a.Balance)
	}
}

func TestPremium_withdraw_adds_the_fee(t *testing.T) {
	p := NewPremium("cy", 100, 5)
	if !p.Withdraw(20) || p.Balance != 75 {
		t.Errorf("Withdraw(20) with fee 5 from 100: balance %d; want 75", p.Balance)
	}
	if p.Withdraw(71) {
		t.Errorf("Withdraw(71) + fee 5 = 76 exceeds balance 75, but returned true")
	}
	if p.Balance != 75 {
		t.Errorf("a refused premium withdrawal changed the balance to %d", p.Balance)
	}
	want := []string{"cy: withdraw 25", "cy: refused 76"}
	if !reflect.DeepEqual(p.Lines(), want) {
		t.Errorf("lines = %q; want %q", p.Lines(), want)
	}
}

func TestPremium_deposit_is_promoted(t *testing.T) {
	p := NewPremium("di", 0, 1)
	p.Deposit(30)
	if p.Balance != 30 {
		t.Errorf("balance after Deposit(30) = %d; want 30", p.Balance)
	}
	if got := Summary(p); got != "di: deposit 30" {
		t.Errorf("Summary = %q; want %q", got, "di: deposit 30")
	}
}

func TestSummary_joins_lines(t *testing.T) {
	a := NewAccount("ed", 5)
	if got := Summary(a); got != "" {
		t.Errorf("Summary with no lines = %q; want \"\"", got)
	}
	a.Deposit(1)
	a.Withdraw(100)
	if got, want := Summary(a), "ed: deposit 1; ed: refused 100"; got != want {
		t.Errorf("Summary = %q; want %q", got, want)
	}
}
