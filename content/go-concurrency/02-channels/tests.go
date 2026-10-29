package main

import (
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
		t.Fatalf("%s did not finish within %v (a channel that is never closed, or a receive that waits too long?)", what, d)
	}
}

func TestGen(t *testing.T) {
	var ch <-chan int
	within(t, time.Second, "Gen(5) returning", func() { ch = Gen(5) })
	var got []int
	within(t, 2*time.Second, "ranging over Gen(5)", func() {
		for v := range ch {
			got = append(got, v)
		}
	})
	if want := []int{1, 2, 3, 4, 5}; !reflect.DeepEqual(got, want) {
		t.Errorf("Gen(5) yielded %v; want %v", got, want)
	}
}

func TestGen_zero_is_closed(t *testing.T) {
	for _, n := range []int{0, -2} {
		ch := Gen(n)
		select {
		case v, ok := <-ch:
			if ok {
				t.Errorf("Gen(%d) yielded %d; want a closed channel", n, v)
			}
		case <-time.After(2 * time.Second):
			t.Errorf("Gen(%d) never closed its channel", n)
		}
	}
}

func TestSum(t *testing.T) {
	var got int
	within(t, 2*time.Second, "Sum(Gen(100))", func() { got = Sum(Gen(100)) })
	if got != 5050 {
		t.Errorf("Sum(Gen(100)) = %d; want 5050", got)
	}
	c := make(chan int, 3)
	c <- -4
	c <- 10
	c <- 1
	close(c)
	within(t, 2*time.Second, "Sum(buffered)", func() { got = Sum(c) })
	if got != 7 {
		t.Errorf("Sum of -4, 10, 1 = %d; want 7", got)
	}
}

func TestPreloaded(t *testing.T) {
	var ch <-chan int
	within(t, time.Second, "Preloaded (it must not block: no goroutine needed)", func() { ch = Preloaded(7, 8, 9) })
	if cap(ch) != 3 {
		t.Errorf("cap(Preloaded(7, 8, 9)) = %d; want 3", cap(ch))
	}
	if len(ch) != 3 {
		t.Errorf("len(Preloaded(7, 8, 9)) = %d; want 3 values already queued", len(ch))
	}
	var got []int
	within(t, 2*time.Second, "draining Preloaded", func() {
		for v := range ch {
			got = append(got, v)
		}
	})
	if want := []int{7, 8, 9}; !reflect.DeepEqual(got, want) {
		t.Errorf("Preloaded(7, 8, 9) yielded %v", got)
	}
	empty := Preloaded()
	select {
	case _, ok := <-empty:
		if ok {
			t.Errorf("Preloaded() should be closed and empty")
		}
	case <-time.After(time.Second):
		t.Errorf("Preloaded() with no values must already be closed")
	}
}

func TestTake_stops_after_n(t *testing.T) {
	ch := make(chan int) // never closed: Take must stop on its own after n values
	go func() {
		for i := 1; ; i++ {
			select {
			case ch <- i:
			case <-time.After(3 * time.Second):
				return
			}
		}
	}()
	var got []int
	within(t, 2*time.Second, "Take(ch, 3)", func() { got = Take(ch, 3) })
	if want := []int{1, 2, 3}; !reflect.DeepEqual(got, want) {
		t.Errorf("Take(ch, 3) = %v; want %v", got, want)
	}
}

func TestTake_closed_early(t *testing.T) {
	var got []int
	within(t, 2*time.Second, "Take on a short channel", func() { got = Take(Gen(2), 10) })
	if want := []int{1, 2}; !reflect.DeepEqual(got, want) {
		t.Errorf("Take(Gen(2), 10) = %v; want %v", got, want)
	}
	within(t, 2*time.Second, "Take on an empty channel", func() { got = Take(Gen(0), 4) })
	if got == nil || len(got) != 0 {
		t.Errorf("Take of nothing = %#v; want an empty, non-nil slice", got)
	}
	within(t, 2*time.Second, "Take(…, 0)", func() { got = Take(make(chan int), 0) })
	if got == nil || len(got) != 0 {
		t.Errorf("Take(ch, 0) = %#v; want an empty, non-nil slice", got)
	}
}
