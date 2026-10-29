package main

import (
	"context"
	"errors"
	"sync"
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
		t.Fatalf("%s did not finish within %v", what, d)
	}
}

func TestLimiter_Do_never_exceeds_limit(t *testing.T) {
	l := NewLimiter(3)
	if l == nil {
		t.Fatal("NewLimiter returned nil")
	}
	var running, peak int32
	var wg sync.WaitGroup
	within(t, 6*time.Second, "Do from 20 goroutines", func() {
		for i := 0; i < 20; i++ {
			wg.Add(1)
			go func() {
				defer wg.Done()
				l.Do(func() {
					cur := atomic.AddInt32(&running, 1)
					for {
						p := atomic.LoadInt32(&peak)
						if cur <= p || atomic.CompareAndSwapInt32(&peak, p, cur) {
							break
						}
					}
					time.Sleep(25 * time.Millisecond)
					atomic.AddInt32(&running, -1)
				})
			}()
		}
		wg.Wait()
	})
	if p := atomic.LoadInt32(&peak); p > 3 {
		t.Errorf("%d goroutines were inside Do at once; the limit is 3", p)
	} else if p < 2 {
		t.Errorf("only %d goroutine was inside Do at a time; a limit of 3 should allow overlap", p)
	}
}

func TestLimiter_TryAcquire(t *testing.T) {
	l := NewLimiter(2)
	var a, b, c bool
	within(t, time.Second, "TryAcquire (must never wait)", func() {
		a, b, c = l.TryAcquire(), l.TryAcquire(), l.TryAcquire()
	})
	if !a || !b || c {
		t.Errorf("TryAcquire x3 on a limit of 2 = %v %v %v; want true true false", a, b, c)
	}
	l.Release()
	if !l.TryAcquire() {
		t.Errorf("TryAcquire should succeed after a Release")
	}
}

func TestLimiter_Acquire_blocks_until_release(t *testing.T) {
	l := NewLimiter(1)
	l.Acquire()
	got := make(chan struct{})
	go func() {
		l.Acquire()
		close(got)
	}()
	select {
	case <-got:
		t.Fatalf("a second Acquire succeeded while the only slot was held")
	case <-time.After(100 * time.Millisecond):
	}
	l.Release()
	select {
	case <-got:
	case <-time.After(3 * time.Second):
		t.Errorf("the waiting Acquire never proceeded after Release")
	}
}

func TestLimiter_AcquireCtx(t *testing.T) {
	l := NewLimiter(1)
	if err := l.AcquireCtx(context.Background()); err != nil {
		t.Fatalf("AcquireCtx on a free limiter = %v; want nil", err)
	}
	ctx, cancel := context.WithTimeout(context.Background(), 60*time.Millisecond)
	defer cancel()
	var err error
	within(t, 3*time.Second, "AcquireCtx on a full limiter", func() { err = l.AcquireCtx(ctx) })
	if !errors.Is(err, context.DeadlineExceeded) {
		t.Errorf("AcquireCtx that timed out = %v; want context.DeadlineExceeded", err)
	}
	// The failed call must not have consumed a slot: releasing the first holder frees one.
	l.Release()
	if !l.TryAcquire() {
		t.Errorf("the slot was not free after Release; did the failed AcquireCtx take a slot?")
	}
	if l.TryAcquire() {
		t.Errorf("a limit of 1 allowed two holders")
	}
}

func TestLimiter_Do_releases_on_panic(t *testing.T) {
	l := NewLimiter(1)
	func() {
		defer func() { recover() }()
		l.Do(func() { panic("boom") })
	}()
	if !l.TryAcquire() {
		t.Errorf("Do did not release its slot after f panicked (use defer)")
	}
}

func TestLimiter_nonpositive_limit(t *testing.T) {
	l := NewLimiter(0)
	if !l.TryAcquire() {
		t.Errorf("NewLimiter(0) should behave like a limit of 1")
	}
	if l.TryAcquire() {
		t.Errorf("NewLimiter(0) allowed two holders")
	}
}
