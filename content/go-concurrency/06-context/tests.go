package main

import (
	"context"
	"errors"
	"testing"
	"time"
)

func within(t *testing.T, d time.Duration, what string, fn func()) {
	t.Helper()
	done := make(chan struct{})
	go func() { fn(); close(done) }()
	select {
	case <-done:
	case <-time.After(d):
		t.Fatalf("%s did not finish within %v (does every blocking step also watch ctx.Done()?)", what, d)
	}
}

func TestSleepCtx_full_sleep(t *testing.T) {
	var err error
	start := time.Now()
	within(t, 3*time.Second, "SleepCtx", func() { err = SleepCtx(context.Background(), 60*time.Millisecond) })
	if err != nil {
		t.Errorf("SleepCtx with a live context = %v; want nil", err)
	}
	if el := time.Since(start); el < 45*time.Millisecond {
		t.Errorf("SleepCtx returned after %v; it should sleep the full 60ms", el)
	}
}

func TestSleepCtx_cancelled(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	go func() {
		time.Sleep(40 * time.Millisecond)
		cancel()
	}()
	var err error
	within(t, 3*time.Second, "SleepCtx(ctx, 1 minute) after cancel", func() { err = SleepCtx(ctx, time.Minute) })
	if !errors.Is(err, context.Canceled) {
		t.Errorf("SleepCtx after cancel = %v; want context.Canceled", err)
	}
}

func TestSleepCtx_deadline(t *testing.T) {
	ctx, cancel := context.WithTimeout(context.Background(), 40*time.Millisecond)
	defer cancel()
	var err error
	within(t, 3*time.Second, "SleepCtx with deadline", func() { err = SleepCtx(ctx, time.Minute) })
	if !errors.Is(err, context.DeadlineExceeded) {
		t.Errorf("SleepCtx past a deadline = %v; want context.DeadlineExceeded", err)
	}
}

func TestSleepCtx_already_cancelled(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	cancel()
	var err error
	within(t, 3*time.Second, "SleepCtx on a dead context", func() { err = SleepCtx(ctx, time.Minute) })
	if !errors.Is(err, context.Canceled) {
		t.Errorf("SleepCtx with a cancelled context = %v; want context.Canceled", err)
	}
}

func TestTicks_counts_then_closes_on_cancel(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()
	ch := Ticks(ctx)
	for want := 0; want < 5; want++ {
		select {
		case v, ok := <-ch:
			if !ok || v != want {
				t.Fatalf("Ticks yielded %d (ok=%v); want %d", v, ok, want)
			}
		case <-time.After(2 * time.Second):
			t.Fatalf("Ticks produced nothing for 2s")
		}
	}
	cancel()
	// After cancel the channel must close; at most a few buffered-in-flight values may still arrive.
	within(t, 3*time.Second, "waiting for Ticks to close after cancel", func() {
		for range ch {
		}
	})
}

func TestTicks_stops_even_if_nobody_reads(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	ch := Ticks(ctx)
	<-ch // take one value, then walk away
	cancel()
	time.Sleep(50 * time.Millisecond)
	// A correct goroutine has exited and closed ch. Draining must finish quickly.
	within(t, 3*time.Second, "draining a cancelled Ticks", func() {
		for range ch {
		}
	})
}

func TestTicks_already_cancelled_context(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	cancel()
	ch := Ticks(ctx)
	within(t, 3*time.Second, "Ticks with a dead context", func() {
		for range ch {
		}
	})
}

func TestCallWithTimeout_success(t *testing.T) {
	var v int
	var err error
	within(t, 3*time.Second, "CallWithTimeout", func() {
		v, err = CallWithTimeout(2*time.Second, func(ctx context.Context) (int, error) { return 41 + 1, nil })
	})
	if v != 42 || err != nil {
		t.Errorf("CallWithTimeout = %d, %v; want 42, nil", v, err)
	}
}

func TestCallWithTimeout_deadline_reaches_f(t *testing.T) {
	var v int
	var err error
	start := time.Now()
	within(t, 4*time.Second, "CallWithTimeout", func() {
		v, err = CallWithTimeout(80*time.Millisecond, func(ctx context.Context) (int, error) {
			select {
			case <-ctx.Done():
				return -1, ctx.Err()
			case <-time.After(time.Minute):
				return 1, nil
			}
		})
	})
	if v != -1 || !errors.Is(err, context.DeadlineExceeded) {
		t.Errorf("CallWithTimeout = %d, %v; want -1, context.DeadlineExceeded", v, err)
	}
	if el := time.Since(start); el < 60*time.Millisecond {
		t.Errorf("returned after %v; the context should live for about 80ms", el)
	}
}

func TestCallWithTimeout_cancels_context_afterwards(t *testing.T) {
	var saved context.Context
	CallWithTimeout(time.Hour, func(ctx context.Context) (int, error) {
		saved = ctx
		return 0, nil
	})
	if saved == nil {
		t.Fatal("f was never called")
	}
	if saved.Err() == nil {
		t.Errorf("the context was still alive after CallWithTimeout returned; call the cancel function (defer cancel())")
	}
}
