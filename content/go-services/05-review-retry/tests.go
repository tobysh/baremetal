package main

import (
	"context"
	"errors"
	"strings"
	"testing"
	"time"
)

func runRetry(t *testing.T, ctx context.Context, attempts int, backoff, per time.Duration, f func(context.Context) error) error {
	t.Helper()
	res := make(chan error, 1)
	go func() { res <- Retry(ctx, attempts, backoff, per, f) }()
	select {
	case err := <-res:
		return err
	case <-time.After(5 * time.Second):
		t.Fatal("Retry did not return (does the backoff wait watch ctx.Done()? does each attempt get its timeout?)")
		return nil
	}
}

func TestRetry_succeeds_after_failures(t *testing.T) {
	for _, failFirst := range []int{0, 1, 3} {
		calls := 0
		err := runRetry(t, context.Background(), 5, 0, 0, func(ctx context.Context) error {
			calls++
			if calls <= failFirst {
				return errors.New("nope")
			}
			return nil
		})
		if err != nil || calls != failFirst+1 {
			t.Errorf("failFirst=%d: err=%v calls=%d; want nil and %d calls", failFirst, err, calls, failFirst+1)
		}
	}
}

func TestRetry_gives_up_with_wrapped_error(t *testing.T) {
	boom := errors.New("boom")
	for _, n := range []int{1, 3, 4} {
		calls := 0
		err := runRetry(t, context.Background(), n, 0, 0, func(ctx context.Context) error { calls++; return boom })
		if calls != n {
			t.Errorf("attempts=%d made %d calls", n, calls)
		}
		if !errors.Is(err, boom) {
			t.Errorf("attempts=%d: %v does not wrap the last error", n, err)
		}
		if err != nil && !strings.HasPrefix(err.Error(), "after "+string(rune('0'+n))+" attempts: ") {
			t.Errorf("attempts=%d: error text %q lacks the \"after N attempts: \" prefix", n, err)
		}
	}
}

func TestRetry_attempts_below_one_means_one(t *testing.T) {
	calls := 0
	err := runRetry(t, context.Background(), 0, 0, 0, func(ctx context.Context) error { calls++; return errors.New("x") })
	if calls != 1 || err == nil {
		t.Errorf("calls=%d err=%v; want 1 call and an error", calls, err)
	}
}

func TestRetry_reports_the_last_error(t *testing.T) {
	e1, e2 := errors.New("first"), errors.New("second")
	n := 0
	err := runRetry(t, context.Background(), 2, 0, 0, func(ctx context.Context) error {
		n++
		if n == 1 {
			return e1
		}
		return e2
	})
	if !errors.Is(err, e2) || errors.Is(err, e1) {
		t.Errorf("err = %v; want it to wrap only the last error", err)
	}
}

func TestRetry_per_attempt_timeout_then_success(t *testing.T) {
	calls := 0
	seen := map[context.Context]bool{}
	err := runRetry(t, context.Background(), 5, 0, 5*time.Millisecond, func(ctx context.Context) error {
		calls++
		seen[ctx] = true
		if _, ok := ctx.Deadline(); !ok {
			t.Error("attempt context has no deadline although perAttempt > 0")
		}
		if calls <= 2 {
			<-ctx.Done() // only the attempt's own timeout can end this
			return ctx.Err()
		}
		return nil
	})
	if err != nil || calls != 3 || len(seen) != 3 {
		t.Errorf("err=%v calls=%d distinct contexts=%d; want nil, 3, 3", err, calls, len(seen))
	}
}

func TestRetry_no_timeout_when_per_attempt_is_zero(t *testing.T) {
	Retry(context.Background(), 1, 0, 0, func(ctx context.Context) error {
		if _, ok := ctx.Deadline(); ok {
			t.Error("perAttempt = 0 should not add a deadline")
		}
		return nil
	})
}

func TestRetry_caller_cancel_during_backoff(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	calls := 0
	err := runRetry(t, ctx, 10, time.Hour, 0, func(c context.Context) error {
		calls++
		cancel() // cancelled while Retry is about to sleep for an hour
		return errors.New("fail")
	})
	if !errors.Is(err, context.Canceled) {
		t.Errorf("err = %v; want context.Canceled", err)
	}
	if calls != 1 {
		t.Errorf("f was called %d times; want 1", calls)
	}
}

func TestRetry_already_cancelled_never_calls_f(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	cancel()
	err := runRetry(t, ctx, 3, 0, 0, func(c context.Context) error {
		t.Error("f was called with a cancelled context")
		return nil
	})
	if !errors.Is(err, context.Canceled) {
		t.Errorf("err = %v; want context.Canceled", err)
	}
}

func TestRetry_overall_deadline_wins(t *testing.T) {
	ctx, cancel := context.WithTimeout(context.Background(), 20*time.Millisecond)
	defer cancel()
	err := runRetry(t, ctx, 1000, time.Hour, 0, func(c context.Context) error { return errors.New("fail") })
	if !errors.Is(err, context.DeadlineExceeded) {
		t.Errorf("err = %v; want DeadlineExceeded", err)
	}
}
