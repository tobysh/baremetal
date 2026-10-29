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
		t.Fatalf("%s did not finish within %v", what, d)
	}
}

func TestCallWithTimeout_returns_result(t *testing.T) {
	for _, want := range []string{"alpha", "beta"} {
		got, err := CallWithTimeout(time.Hour, func(ctx context.Context) (string, error) { return want, nil })
		if err != nil || got != want {
			t.Errorf("CallWithTimeout = %q, %v; want %q, nil", got, err, want)
		}
	}
}

func TestCallWithTimeout_passes_error_through(t *testing.T) {
	boom := errors.New("boom")
	_, err := CallWithTimeout(time.Hour, func(ctx context.Context) (string, error) { return "", boom })
	if !errors.Is(err, boom) {
		t.Errorf("err = %v; want boom", err)
	}
}

func TestCallWithTimeout_f_sees_a_deadline(t *testing.T) {
	CallWithTimeout(time.Hour, func(ctx context.Context) (string, error) {
		if _, ok := ctx.Deadline(); !ok {
			t.Error("the context given to f has no deadline")
		}
		return "", nil
	})
}

func TestCallWithTimeout_cooperative_f(t *testing.T) {
	var err error
	within(t, 3*time.Second, "CallWithTimeout", func() {
		_, err = CallWithTimeout(10*time.Millisecond, func(ctx context.Context) (string, error) {
			<-ctx.Done()
			return "", ctx.Err()
		})
	})
	if !errors.Is(err, context.DeadlineExceeded) {
		t.Errorf("err = %v; want DeadlineExceeded", err)
	}
}

func TestCallWithTimeout_stops_waiting_for_f_that_ignores_ctx(t *testing.T) {
	release := make(chan struct{})
	finished := make(chan struct{})
	var s string
	var err error
	within(t, 3*time.Second, "CallWithTimeout with a stubborn f", func() {
		s, err = CallWithTimeout(10*time.Millisecond, func(ctx context.Context) (string, error) {
			defer close(finished)
			<-release // ignores ctx entirely
			return "late", nil
		})
	})
	if !errors.Is(err, context.DeadlineExceeded) || s != "" {
		t.Errorf("got %q, %v; want \"\" and DeadlineExceeded", s, err)
	}
	close(release)
	select {
	case <-finished:
	case <-time.After(2 * time.Second):
		t.Error("f never finished after release")
	}
}

func TestRemaining(t *testing.T) {
	if d, ok := Remaining(context.Background()); ok || d != 0 {
		t.Errorf("Remaining(no deadline) = %v, %v; want 0, false", d, ok)
	}
	ctx, cancel := context.WithTimeout(context.Background(), time.Hour)
	defer cancel()
	d, ok := Remaining(ctx)
	if !ok || d <= 59*time.Minute || d > time.Hour {
		t.Errorf("Remaining(1h) = %v, %v; want just under 1h, true", d, ok)
	}
	past, cancel2 := context.WithDeadline(context.Background(), time.Now().Add(-time.Hour))
	defer cancel2()
	if d, ok := Remaining(past); !ok || d != 0 {
		t.Errorf("Remaining(past) = %v, %v; want 0, true", d, ok)
	}
}

func TestHalf(t *testing.T) {
	parent, cancel := context.WithTimeout(context.Background(), time.Hour)
	defer cancel()
	child, cancelChild := Half(parent)
	defer cancelChild()
	d, ok := Remaining(child)
	if !ok || d > 30*time.Minute || d < 29*time.Minute {
		t.Errorf("Half of 1h left %v (ok=%v); want about 30m", d, ok)
	}
	if _, ok := Remaining(parent); !ok {
		t.Error("the parent lost its deadline")
	}
}

func TestHalf_without_deadline(t *testing.T) {
	child, cancel := Half(context.Background())
	if cancel == nil {
		t.Fatal("Half returned a nil cancel function")
	}
	if _, ok := child.Deadline(); ok {
		t.Error("a child of a deadline-less context must not get a deadline")
	}
	cancel()
	if child.Err() == nil {
		t.Error("calling the cancel function should cancel the child")
	}
}
