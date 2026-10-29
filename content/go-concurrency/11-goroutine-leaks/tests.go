package main

import (
	"runtime"
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

// settle waits (up to 3s) for the goroutine count to drop to at most base.
func settle(base int) int {
	n := runtime.NumGoroutine()
	for deadline := time.Now().Add(3 * time.Second); n > base && time.Now().Before(deadline); {
		time.Sleep(10 * time.Millisecond)
		n = runtime.NumGoroutine()
	}
	return n
}

func TestFirst_returns_fastest(t *testing.T) {
	// Only the chosen function can return until First has produced its result.
	// Vary its index so a sequential or first-argument implementation cannot pass.
	for winner := 0; winner < 3; winner++ {
		func() {
			release := make(chan struct{})
			defer close(release)
			functions := make([]func() int, 3)
			for i := range functions {
				functions[i] = func() int {
					if i != winner {
						<-release
					}
					return 10 + i
				}
			}
			var got int
			within(t, 3*time.Second, "First with blocked competitors", func() { got = First(functions...) })
			if got != 10+winner {
				t.Errorf("First = %d; want ready function %d", got, 10+winner)
			}
		}()
	}
}

func TestFirst_does_not_wait_for_the_others(t *testing.T) {
	release := make(chan struct{})
	defer close(release)
	var got int
	within(t, 3*time.Second, "First before slower task is released", func() {
		got = First(func() int { return 7 }, func() int { <-release; return 8 })
	})
	if got != 7 {
		t.Errorf("First = %d; want 7", got)
	}
}

func TestFirst_no_functions(t *testing.T) {
	var got int
	within(t, time.Second, "First()", func() { got = First() })
	if got != 0 {
		t.Errorf("First() = %d; want 0", got)
	}
}

func TestFirst_leaves_no_goroutines_behind(t *testing.T) {
	time.Sleep(50 * time.Millisecond)
	base := runtime.NumGoroutine()
	for i := 0; i < 20; i++ {
		First(
			func() int { return 1 },
			func() int { time.Sleep(60 * time.Millisecond); return 2 },
			func() int { time.Sleep(90 * time.Millisecond); return 3 },
		)
	}
	if n := settle(base); n > base {
		t.Errorf("%d goroutines are still alive (%d before the calls). The slower functions finish but their sends block forever; give the channel room for every result", n, base)
	}
}

func TestGenerator_yields_counting_numbers(t *testing.T) {
	done := make(chan struct{})
	defer close(done)
	ch := Generator(done)
	for want := 0; want < 6; want++ {
		select {
		case v, ok := <-ch:
			if !ok || v != want {
				t.Fatalf("Generator yielded %d (ok=%v); want %d", v, ok, want)
			}
		case <-time.After(2 * time.Second):
			t.Fatalf("Generator produced nothing for 2s")
		}
	}
}

func TestGenerator_closes_after_done_even_unread(t *testing.T) {
	done := make(chan struct{})
	ch := Generator(done)
	<-ch
	close(done)
	// Never drain; the only way the channel can become closed is the producer exiting.
	// Give it time, then receive until closed (at most one in-flight value may appear).
	closed := false
	deadline := time.Now().Add(3 * time.Second)
	for !closed && time.Now().Before(deadline) {
		select {
		case _, ok := <-ch:
			if !ok {
				closed = true
			}
		case <-time.After(20 * time.Millisecond):
		}
	}
	if !closed {
		t.Errorf("Generator did not close its output after done was closed")
	}
}

func TestGenerator_leaves_no_goroutines_behind(t *testing.T) {
	time.Sleep(50 * time.Millisecond)
	base := runtime.NumGoroutine()
	for i := 0; i < 20; i++ {
		done := make(chan struct{})
		ch := Generator(done)
		<-ch
		<-ch
		close(done) // walk away without draining
	}
	if n := settle(base); n > base {
		t.Errorf("%d goroutines are still alive (%d before). The producer blocks on its send once the reader leaves; select on done as well", n, base)
	}
}
