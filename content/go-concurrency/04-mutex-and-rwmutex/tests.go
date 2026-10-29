package main

import (
	"sync"
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
		t.Fatalf("%s did not finish within %v (a lock that is never released, or locked twice?)", what, d)
	}
}

func hammer(goroutines int, fn func(g int)) {
	var wg sync.WaitGroup
	start := make(chan struct{})
	for g := 0; g < goroutines; g++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			<-start
			fn(g)
		}()
	}
	close(start)
	wg.Wait()
}

func TestCounter_zero_value_and_sequential(t *testing.T) {
	var c Counter
	if c.Value() != 0 {
		t.Fatalf("a new Counter should start at 0, got %d", c.Value())
	}
	c.Inc()
	c.Inc()
	c.Inc()
	if c.Value() != 3 {
		t.Errorf("after 3 Inc calls Value() = %d; want 3", c.Value())
	}
}

func TestCounter_concurrent(t *testing.T) {
	var c Counter
	within(t, 5*time.Second, "concurrent Inc", func() {
		hammer(50, func(int) {
			for i := 0; i < 2000; i++ {
				c.Inc()
			}
		})
	})
	if v := c.Value(); v != 100000 {
		t.Errorf("after 50 goroutines x 2000 Inc, Value() = %d; want 100000 (lost updates mean a missing lock)", v)
	}
}

func TestCounter_reads_while_writing(t *testing.T) {
	var c Counter
	within(t, 5*time.Second, "mixed Inc and Value", func() {
		hammer(20, func(g int) {
			for i := 0; i < 1000; i++ {
				if g%2 == 0 {
					c.Inc()
				} else {
					_ = c.Value()
				}
			}
		})
	})
	if v := c.Value(); v != 10000 {
		t.Errorf("Value() = %d; want 10000", v)
	}
}

func TestRegistry_basic(t *testing.T) {
	r := NewRegistry()
	if r == nil {
		t.Fatal("NewRegistry returned nil")
	}
	if _, ok := r.Get("x"); ok {
		t.Errorf("Get of a missing key reported ok")
	}
	r.Set("x", 4)
	r.Set("y", 9)
	if v, ok := r.Get("x"); !ok || v != 4 {
		t.Errorf("Get(x) = %d, %v; want 4, true", v, ok)
	}
	if r.Len() != 2 {
		t.Errorf("Len() = %d; want 2", r.Len())
	}
	if n := r.Incr("x"); n != 5 {
		t.Errorf("Incr(x) = %d; want 5", n)
	}
	if n := r.Incr("new"); n != 1 {
		t.Errorf("Incr of a missing key = %d; want 1", n)
	}
	if r.Len() != 3 {
		t.Errorf("Len() = %d; want 3 after Incr created a key", r.Len())
	}
}

func TestRegistry_concurrent_incr(t *testing.T) {
	r := NewRegistry()
	seen := make([]map[int]bool, 20)
	within(t, 5*time.Second, "concurrent Incr", func() {
		hammer(20, func(g int) {
			seen[g] = map[int]bool{}
			for i := 0; i < 500; i++ {
				seen[g][r.Incr("hits")] = true
			}
		})
	})
	if v, _ := r.Get("hits"); v != 10000 {
		t.Errorf("hits = %d; want 10000 (is Incr one atomic read-modify-write?)", v)
	}
	// every returned value must be unique across goroutines
	all := map[int]int{}
	for _, m := range seen {
		for n := range m {
			all[n]++
		}
	}
	for n, c := range all {
		if c > 1 {
			t.Errorf("Incr returned %d to %d different goroutines; each call must see a distinct new value", n, c)
			break
		}
	}
}

func TestRegistry_concurrent_readers_and_writers(t *testing.T) {
	r := NewRegistry()
	within(t, 5*time.Second, "mixed Set/Get/Len", func() {
		hammer(16, func(g int) {
			for i := 0; i < 500; i++ {
				switch g % 3 {
				case 0:
					r.Set("k", i)
				case 1:
					r.Get("k")
				default:
					r.Len()
				}
			}
		})
	})
	if r.Len() != 1 {
		t.Errorf("Len() = %d; want 1", r.Len())
	}
}
