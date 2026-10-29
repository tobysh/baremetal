package main

import (
	"context"
	"reflect"
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

func fill(vals ...int) <-chan int {
	c := make(chan int, len(vals))
	for _, v := range vals {
		c <- v
	}
	close(c)
	return c
}

func TestRateLimited_order_and_close(t *testing.T) {
	var got []int
	var out <-chan int
	within(t, time.Second, "RateLimited must return immediately", func() { out = RateLimited(fill(1, 2, 3, 4, 5), 20*time.Millisecond) })
	within(t, 5*time.Second, "reading RateLimited", func() {
		for v := range out {
			got = append(got, v)
		}
	})
	if want := []int{1, 2, 3, 4, 5}; !reflect.DeepEqual(got, want) {
		t.Errorf("RateLimited yielded %v; want %v", got, want)
	}
}

func TestRateLimited_is_paced(t *testing.T) {
	const n = 6
	vals := make([]int, n)
	start := time.Now()
	var count int
	within(t, 5*time.Second, "reading RateLimited", func() {
		for range RateLimited(fill(vals...), 40*time.Millisecond) {
			count++
		}
	})
	el := time.Since(start)
	if count != n {
		t.Fatalf("got %d values; want %d", count, n)
	}
	// 6 values at 40ms: about 240ms. Allow a wide margin below for scheduler jitter.
	if el < 150*time.Millisecond {
		t.Errorf("6 values at one per 40ms took only %v; the output is not being paced by the ticker", el)
	}
}

func TestRateLimited_empty_input_closes(t *testing.T) {
	var got []int
	within(t, 3*time.Second, "RateLimited(empty)", func() {
		for v := range RateLimited(fill(), 30*time.Millisecond) {
			got = append(got, v)
		}
	})
	if len(got) != 0 {
		t.Errorf("got %v from an empty input", got)
	}
}

func TestPoll_true_when_condition_met(t *testing.T) {
	calls := 0
	start := time.Now()
	var ok bool
	within(t, 5*time.Second, "Poll", func() {
		ok = Poll(context.Background(), 30*time.Millisecond, func() bool {
			calls++
			return calls == 3
		})
	})
	if !ok {
		t.Errorf("Poll = false; want true once check returned true")
	}
	if calls != 3 {
		t.Errorf("check was called %d times; want exactly 3 (once per tick, stopping at the first true)", calls)
	}
	if el := time.Since(start); el < 60*time.Millisecond {
		t.Errorf("Poll took %v for three 30ms ticks; the first check should wait one interval", el)
	}
}

func TestPoll_first_check_waits_one_interval(t *testing.T) {
	first := time.Duration(0)
	start := time.Now()
	within(t, 3*time.Second, "Poll", func() {
		Poll(context.Background(), 80*time.Millisecond, func() bool {
			first = time.Since(start)
			return true
		})
	})
	if first < 50*time.Millisecond {
		t.Errorf("check ran after only %v; it should first run after one 80ms interval, not immediately", first)
	}
}

func TestPoll_false_when_context_ends(t *testing.T) {
	ctx, cancel := context.WithTimeout(context.Background(), 120*time.Millisecond)
	defer cancel()
	calls := 0
	var ok bool
	within(t, 3*time.Second, "Poll with a deadline", func() {
		ok = Poll(ctx, 20*time.Millisecond, func() bool { calls++; return false })
	})
	if ok {
		t.Errorf("Poll = true; want false when check never succeeds")
	}
	if calls < 1 || calls > 7 {
		t.Errorf("check ran %d times in 120ms at a 20ms beat; expected a handful", calls)
	}
}

func TestPoll_cancelled_before_start(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	cancel()
	var ok bool
	within(t, 3*time.Second, "Poll with a dead context", func() {
		ok = Poll(ctx, time.Hour, func() bool { return true })
	})
	if ok {
		t.Errorf("Poll with an already-cancelled context returned true")
	}
}
