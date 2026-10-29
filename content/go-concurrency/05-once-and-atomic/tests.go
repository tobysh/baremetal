package main

import (
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

func TestLazy_sequential(t *testing.T) {
	var l Lazy
	calls := 0
	a := l.Get(func() int { calls++; return 42 })
	b := l.Get(func() int { calls++; return 99 })
	if a != 42 || b != 42 {
		t.Errorf("Get returned %d then %d; want 42 both times (the first result is remembered)", a, b)
	}
	if calls != 1 {
		t.Errorf("init ran %d times; want exactly 1", calls)
	}
}

func TestLazy_concurrent_init_runs_once_and_callers_wait(t *testing.T) {
	var l Lazy
	var calls int32
	results := make([]int, 40)
	var wg sync.WaitGroup
	within(t, 5*time.Second, "concurrent Lazy.Get", func() {
		for i := range results {
			wg.Add(1)
			go func() {
				defer wg.Done()
				results[i] = l.Get(func() int {
					atomic.AddInt32(&calls, 1)
					time.Sleep(50 * time.Millisecond) // slow init: others must wait for it
					return 1234
				})
			}()
		}
		wg.Wait()
	})
	if c := atomic.LoadInt32(&calls); c != 1 {
		t.Errorf("init ran %d times under concurrency; want exactly 1", c)
	}
	for i, r := range results {
		if r != 1234 {
			t.Errorf("caller %d got %d; a caller must wait for the first init to finish and see 1234", i, r)
			break
		}
	}
}

func TestStats_sequential(t *testing.T) {
	var s Stats
	if s.Count() != 0 || s.Total() != 0 || s.Max() != 0 {
		t.Fatalf("a new Stats should be all zeros, got %d %d %d", s.Count(), s.Total(), s.Max())
	}
	for _, v := range []int64{5, 12, 3} {
		s.Record(v)
	}
	if s.Count() != 3 || s.Total() != 20 || s.Max() != 12 {
		t.Errorf("after 5, 12, 3: Count=%d Total=%d Max=%d; want 3 20 12", s.Count(), s.Total(), s.Max())
	}
}

func TestStats_concurrent(t *testing.T) {
	var s Stats
	const goroutines, per = 20, 500
	var wg sync.WaitGroup
	within(t, 5*time.Second, "concurrent Record", func() {
		for g := 0; g < goroutines; g++ {
			wg.Add(1)
			go func() {
				defer wg.Done()
				for i := 1; i <= per; i++ {
					// goroutine g records g*per+i, so values are 1..10000 each exactly once
					s.Record(int64(g*per + i))
				}
			}()
		}
		wg.Wait()
	})
	if s.Count() != goroutines*per {
		t.Errorf("Count() = %d; want %d", s.Count(), goroutines*per)
	}
	if want := int64(goroutines*per) * int64(goroutines*per+1) / 2; s.Total() != want {
		t.Errorf("Total() = %d; want %d", s.Total(), want)
	}
	if s.Max() != goroutines*per {
		t.Errorf("Max() = %d; want %d (a check-then-store race can lose the true maximum)", s.Max(), goroutines*per)
	}
}

func TestStats_max_with_decreasing_values(t *testing.T) {
	var s Stats
	var wg sync.WaitGroup
	for i := 0; i < 200; i++ {
		wg.Add(1)
		go func() { defer wg.Done(); s.Record(int64(1000 - i)) }()
	}
	wg.Wait()
	if s.Max() != 1000 {
		t.Errorf("Max() = %d; want 1000", s.Max())
	}
}
