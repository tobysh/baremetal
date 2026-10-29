package main

import (
	"context"
	"testing"
	"time"
)

func TestAbortable_cause(t *testing.T) {
	for _, reason := range []string{"disk full", "user left", "x"} {
		ctx, abort := NewAbortable(context.Background())
		if ctx.Err() != nil {
			t.Fatal("context is cancelled before abort")
		}
		abort(reason)
		<-ctx.Done()
		if ctx.Err() != context.Canceled {
			t.Errorf("Err = %v; want context.Canceled", ctx.Err())
		}
		if c := context.Cause(ctx); c == nil || c.Error() != reason {
			t.Errorf("Cause = %v; want %q", c, reason)
		}
	}
}

func TestAbortable_first_call_wins(t *testing.T) {
	ctx, abort := NewAbortable(context.Background())
	abort("first")
	abort("second")
	if got := Why(ctx); got != "first" {
		t.Errorf("Why = %q; want first", got)
	}
}

func TestAbortable_follows_parent(t *testing.T) {
	parent, cancel := context.WithCancel(context.Background())
	ctx, _ := NewAbortable(parent)
	cancel()
	<-ctx.Done()
	if got := Why(ctx); got != "canceled" {
		t.Errorf("Why(child of cancelled parent) = %q; want canceled", got)
	}
}

func TestWhy_states(t *testing.T) {
	live, cancelLive := context.WithCancel(context.Background())
	defer cancelLive()
	if got := Why(live); got != "" {
		t.Errorf("Why(live) = %q; want empty", got)
	}
	if got := Why(context.Background()); got != "" {
		t.Errorf("Why(Background) = %q; want empty", got)
	}
	plain, cancel := context.WithCancel(context.Background())
	cancel()
	if got := Why(plain); got != "canceled" {
		t.Errorf("Why(plain cancel) = %q; want canceled", got)
	}
	timed, cancelT := context.WithTimeout(context.Background(), time.Nanosecond)
	defer cancelT()
	<-timed.Done()
	if got := Why(timed); got != "timed out" {
		t.Errorf("Why(plain deadline) = %q; want timed out", got)
	}
}

func TestTimeoutWithCause(t *testing.T) {
	for _, what := range []string{"database query", "upload"} {
		ctx, cancel := TimeoutWithCause(context.Background(), 5*time.Millisecond, what)
		select {
		case <-ctx.Done():
		case <-time.After(3 * time.Second):
			t.Fatal("the context never timed out")
		}
		cancel()
		if ctx.Err() != context.DeadlineExceeded {
			t.Errorf("Err = %v; want DeadlineExceeded", ctx.Err())
		}
		if got := Why(ctx); got != what+" timed out" {
			t.Errorf("Why = %q; want %q", got, what+" timed out")
		}
	}
}

func TestTimeoutWithCause_early_cancel_is_not_a_timeout(t *testing.T) {
	ctx, cancel := TimeoutWithCause(context.Background(), time.Hour, "slow thing")
	cancel()
	if got := Why(ctx); got != "canceled" {
		t.Errorf("Why after early cancel = %q; want canceled", got)
	}
}

func TestOnCancel_runs_after_cancel(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	ran := make(chan struct{})
	OnCancel(ctx, func() { close(ran) })
	select {
	case <-ran:
		t.Fatal("f ran before the context was cancelled")
	default:
	}
	cancel()
	select {
	case <-ran:
	case <-time.After(3 * time.Second):
		t.Fatal("f did not run after cancel")
	}
}

func TestOnCancel_stop_prevents_the_call(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	ran := make(chan struct{}, 1)
	stop := OnCancel(ctx, func() { ran <- struct{}{} })
	if stop == nil {
		t.Fatal("OnCancel returned a nil stop function")
	}
	if !stop() {
		t.Fatal("stop() = false before cancel; want true")
	}
	cancel()
	// A stopped callback must never run. Give a wrongly-running one a moment to show up.
	select {
	case <-ran:
		t.Error("f ran although stop() returned true")
	case <-time.After(50 * time.Millisecond):
	}
}

func TestOnCancel_stop_after_run_reports_false(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	ran := make(chan struct{})
	stop := OnCancel(ctx, func() { close(ran) })
	cancel()
	<-ran
	if stop() {
		t.Error("stop() = true after f already ran; want false")
	}
}
