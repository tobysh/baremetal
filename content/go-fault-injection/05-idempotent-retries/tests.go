package main

import (
	"errors"
	"testing"
)

const (
	okOut = iota
	lostRequest
	lostReply
)

func schedules(n int) [][]int {
	if n == 0 {
		return [][]int{{}}
	}
	var out [][]int
	for _, s := range schedules(n - 1) {
		for o := okOut; o <= lostReply; o++ {
			out = append(out, append(append([]int{}, s...), o))
		}
	}
	return out
}

func TestPay_every_failure_schedule(t *testing.T) {
	for n := 0; n <= 4; n++ {
		for _, sched := range schedules(n) {
			for max := 0; max <= 4; max++ {
				l := NewLedger(100)
				var ids []string
				call := func(id string, amt int) error {
					i := len(ids)
					ids = append(ids, id)
					o := okOut
					if i < len(sched) {
						o = sched[i]
					}
					switch o {
					case lostRequest:
						return ErrTimeout
					case lostReply:
						l.Debit(id, amt)
						return ErrTimeout
					}
					return l.Debit(id, amt)
				}
				err := Pay(call, "req-42", 30, max)

				// expected outcome, worked out directly from the schedule
				wantCalls, applied, success := 0, false, false
				for i := 0; i < max; i++ {
					wantCalls++
					o := okOut
					if i < len(sched) {
						o = sched[i]
					}
					if o != lostRequest {
						applied = true
					}
					if o == okOut {
						success = true
						break
					}
				}
				wantBalance := 100
				if applied {
					wantBalance = 70
				}
				if len(ids) != wantCalls {
					t.Fatalf("schedule %v budget %d: %d calls, want %d", sched, max, len(ids), wantCalls)
				}
				for _, id := range ids {
					if id != "req-42" {
						t.Fatalf("schedule %v budget %d: attempt used id %q, every attempt must reuse the same id", sched, max, id)
					}
				}
				if l.Balance() != wantBalance {
					t.Fatalf("schedule %v budget %d: balance %d, want %d (effect duplicated or lost)", sched, max, l.Balance(), wantBalance)
				}
				if success && err != nil {
					t.Fatalf("schedule %v budget %d: err = %v, want nil", sched, max, err)
				}
				if !success && !errors.Is(err, ErrGaveUp) {
					t.Fatalf("schedule %v budget %d: err = %v, want ErrGaveUp", sched, max, err)
				}
			}
		}
	}
}

func TestPay_permanent_errors_are_not_retried(t *testing.T) {
	l := NewLedger(10)
	calls := 0
	call := func(id string, amt int) error { calls++; return l.Debit(id, amt) }
	if err := Pay(call, "big", 50, 5); !errors.Is(err, ErrInsufficient) || calls != 1 {
		t.Errorf("err=%v calls=%d, want ErrInsufficient after exactly 1 call", err, calls)
	}
	calls = 0
	if err := Pay(call, "zero", 0, 5); !errors.Is(err, ErrInvalid) || calls != 1 {
		t.Errorf("err=%v calls=%d, want ErrInvalid after exactly 1 call", err, calls)
	}
}

func TestLedger_idempotency_rules(t *testing.T) {
	l := NewLedger(100)
	for i := 0; i < 3; i++ {
		if err := l.Debit("a", 40); err != nil {
			t.Fatalf("repeat %d: %v", i, err)
		}
	}
	if l.Balance() != 60 {
		t.Fatalf("balance %d after the same request 3 times, want 60", l.Balance())
	}
	if err := l.Debit("a", 41); !errors.Is(err, ErrConflict) || l.Balance() != 60 {
		t.Errorf("same id, other amount: err=%v balance=%d", err, l.Balance())
	}
	if err := l.Debit("b", 50); err != nil || l.Balance() != 10 {
		t.Errorf("a new id applies: err=%v balance=%d", err, l.Balance())
	}
	if err := l.Debit("a", 40); err != nil || l.Balance() != 10 {
		t.Errorf("a repeat of an applied id succeeds even when the balance is now too low: err=%v balance=%d", err, l.Balance())
	}
	if err := l.Debit("c", 11); !errors.Is(err, ErrInsufficient) {
		t.Errorf("err=%v, want ErrInsufficient", err)
	}
	if err := l.Debit("c", 10); err != nil || l.Balance() != 0 {
		t.Errorf("a rejected id is not remembered, so it may be retried with another amount: err=%v balance=%d", err, l.Balance())
	}
}
