package main

import "testing"

func TestNewAccount_sets_fields(t *testing.T) {
	a := NewAccount("Ana", 100)
	if a == nil {
		t.Fatalf("NewAccount returned nil; want a pointer to a new Account")
	}
	if a.Owner != "Ana" || a.Balance != 100 {
		t.Errorf("NewAccount(\"Ana\", 100) = %+v; want Owner Ana, Balance 100", *a)
	}
}

func TestDeposit_changes_the_original(t *testing.T) {
	a := NewAccount("Ana", 100)
	a.Deposit(50)
	if a.Balance != 150 {
		t.Errorf("after Deposit(50) on 100, Balance = %d; want 150 (is Deposit using a pointer receiver?)", a.Balance)
	}
}

func TestDeposit_ignores_non_positive(t *testing.T) {
	a := NewAccount("Ana", 100)
	a.Deposit(0)
	a.Deposit(-30)
	if a.Balance != 100 {
		t.Errorf("after Deposit(0) and Deposit(-30), Balance = %d; want 100", a.Balance)
	}
}

func TestWithdraw_success(t *testing.T) {
	a := NewAccount("Bo", 100)
	ok := a.Withdraw(40)
	if !ok || a.Balance != 60 {
		t.Errorf("Withdraw(40) on 100 = %v with Balance %d; want true and 60", ok, a.Balance)
	}
}

func TestWithdraw_refuses_to_overdraw(t *testing.T) {
	a := NewAccount("Bo", 100)
	ok := a.Withdraw(101)
	if ok || a.Balance != 100 {
		t.Errorf("Withdraw(101) on 100 = %v with Balance %d; want false and 100", ok, a.Balance)
	}
}

func TestWithdraw_exact_balance_and_bad_amounts(t *testing.T) {
	a := NewAccount("Bo", 100)
	if a.Withdraw(0) || a.Withdraw(-5) {
		t.Errorf("Withdraw of 0 or a negative amount returned true; want false")
	}
	if !a.Withdraw(100) || a.Balance != 0 {
		t.Errorf("Withdraw(100) on 100 should succeed and leave 0, got Balance %d", a.Balance)
	}
}

func TestSummary_format(t *testing.T) {
	a := NewAccount("Ana", 100)
	if got := a.Summary(); got != "Ana: 100" {
		t.Errorf("Summary() = %q; want %q", got, "Ana: 100")
	}
	var z Account
	if got := z.Summary(); got != ": 0" {
		t.Errorf("zero Account Summary() = %q; want %q", got, ": 0")
	}
}
