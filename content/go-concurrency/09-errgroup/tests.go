package main

import (
	"context"
	"errors"
	"sync/atomic"
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
		t.Fatalf("%s did not finish within %v (Wait must return once every task is done)", what, d)
	}
}

func TestGroup_all_succeed(t *testing.T) {
	g, ctx := WithContext(context.Background())
	var n int32
	for i := 0; i < 10; i++ {
		g.Go(func() error { atomic.AddInt32(&n, 1); return nil })
	}
	var err error
	within(t, 3*time.Second, "Wait", func() { err = g.Wait() })
	if err != nil {
		t.Errorf("Wait = %v; want nil", err)
	}
	if atomic.LoadInt32(&n) != 10 {
		t.Errorf("%d of 10 tasks ran before Wait returned", n)
	}
	if ctx.Err() == nil {
		t.Errorf("the group's context should be cancelled once Wait has returned")
	}
}

func TestGroup_waits_for_slow_tasks(t *testing.T) {
	g, _ := WithContext(context.Background())
	var done int32
	g.Go(func() error { time.Sleep(150 * time.Millisecond); atomic.StoreInt32(&done, 1); return nil })
	within(t, 3*time.Second, "Wait", func() { g.Wait() })
	if atomic.LoadInt32(&done) != 1 {
		t.Errorf("Wait returned before the slow task finished")
	}
}

var errBoom = errors.New("boom")

func TestGroup_returns_first_error_and_cancels_the_rest(t *testing.T) {
	g, ctx := WithContext(context.Background())
	var sawCancel int32
	g.Go(func() error { return errBoom })
	for i := 0; i < 3; i++ {
		g.Go(func() error {
			select {
			case <-ctx.Done():
				atomic.AddInt32(&sawCancel, 1)
				return ctx.Err() // a later error: must NOT replace the first one
			case <-time.After(10 * time.Second):
				return nil
			}
		})
	}
	var err error
	start := time.Now()
	within(t, 5*time.Second, "Wait after a failure", func() { err = g.Wait() })
	if !errors.Is(err, errBoom) {
		t.Errorf("Wait = %v; want the first error (boom), not a later context.Canceled", err)
	}
	if n := atomic.LoadInt32(&sawCancel); n != 3 {
		t.Errorf("%d of 3 waiting tasks saw the context cancelled; the first error should cancel it", n)
	}
	if el := time.Since(start); el > 3*time.Second {
		t.Errorf("Wait took %v; the failure should have stopped the others quickly", el)
	}
}

func TestGroup_first_of_several_errors(t *testing.T) {
	g, _ := WithContext(context.Background())
	first := errors.New("first")
	second := errors.New("second")
	gate := make(chan struct{})
	g.Go(func() error { return first })
	g.Go(func() error { <-gate; return second })
	time.Sleep(100 * time.Millisecond) // let the first error land
	close(gate)
	var err error
	within(t, 3*time.Second, "Wait", func() { err = g.Wait() })
	if err != first {
		t.Errorf("Wait = %v; want the first error", err)
	}
}

func TestGroup_parent_context_flows_through(t *testing.T) {
	parent, cancel := context.WithCancel(context.Background())
	g, ctx := WithContext(parent)
	g.Go(func() error {
		<-ctx.Done()
		return nil
	})
	time.AfterFunc(50*time.Millisecond, cancel)
	var err error
	within(t, 3*time.Second, "Wait with parent cancelled", func() { err = g.Wait() })
	if err != nil {
		t.Errorf("Wait = %v; want nil (the task itself returned nil)", err)
	}
}

func TestGroup_Wait_with_no_tasks(t *testing.T) {
	g, _ := WithContext(context.Background())
	var err error
	within(t, time.Second, "Wait with no tasks", func() { err = g.Wait() })
	if err != nil {
		t.Errorf("Wait with no tasks = %v; want nil", err)
	}
}
