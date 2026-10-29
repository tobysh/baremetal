package main

import (
	"context"
	"errors"
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
		t.Fatalf("%s did not finish within %v (is something blocking without watching ctx.Done()?)", what, d)
	}
}

func TestCounter_counts_from_zero(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()
	ch := Counter(ctx)
	for want := 0; want < 5; want++ {
		select {
		case got := <-ch:
			if got != want {
				t.Fatalf("Counter yielded %d; want %d", got, want)
			}
		case <-time.After(2 * time.Second):
			t.Fatalf("Counter did not yield %d in time", want)
		}
	}
}

func TestCounter_closes_after_cancel(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	ch := Counter(ctx)
	<-ch
	<-ch
	cancel()
	within(t, 2*time.Second, "waiting for the channel to close", func() {
		for range ch {
		}
	})
}

func TestCounter_already_cancelled_closes(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	cancel()
	ch := Counter(ctx)
	within(t, 2*time.Second, "reading a Counter on a cancelled context", func() {
		for range ch {
		}
	})
}

func TestSleep_completes(t *testing.T) {
	if err := Sleep(context.Background(), 5*time.Millisecond); err != nil {
		t.Errorf("Sleep returned %v; want nil", err)
	}
}

func TestSleep_cancelled_while_waiting(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	res := make(chan error, 1)
	started := make(chan struct{})
	go func() {
		close(started)
		res <- Sleep(ctx, time.Hour)
	}()
	<-started
	cancel()
	select {
	case err := <-res:
		if !errors.Is(err, context.Canceled) {
			t.Errorf("Sleep returned %v; want context.Canceled", err)
		}
	case <-time.After(3 * time.Second):
		t.Fatal("Sleep(ctx, 1h) did not return after cancel; use a timer in a select, not time.Sleep")
	}
}

func TestSleep_already_cancelled_even_for_zero(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	cancel()
	for _, d := range []time.Duration{0, time.Millisecond, time.Hour} {
		var err error
		within(t, 2*time.Second, "Sleep on a cancelled context", func() { err = Sleep(ctx, d) })
		if !errors.Is(err, context.Canceled) {
			t.Errorf("Sleep(cancelled, %v) = %v; want context.Canceled", d, err)
		}
	}
}

func TestDrain_until_closed(t *testing.T) {
	in := make(chan int)
	go func() {
		for _, v := range []int{7, 8, 9} {
			in <- v
		}
		close(in)
	}()
	var got []int
	within(t, 2*time.Second, "Drain", func() { got = Drain(context.Background(), in) })
	if !reflect.DeepEqual(got, []int{7, 8, 9}) {
		t.Errorf("Drain = %v; want [7 8 9]", got)
	}
}

func TestDrain_stops_on_cancel_and_empty_is_non_nil(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	in := make(chan int) // never written, never closed
	cancel()
	var got []int
	within(t, 2*time.Second, "Drain with a cancelled context", func() { got = Drain(ctx, in) })
	if got == nil || len(got) != 0 {
		t.Errorf("Drain = %#v; want an empty, non-nil slice", got)
	}
}

func TestDrain_returns_partial_results_on_cancel(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	in := make(chan int)
	res := make(chan []int, 1)
	go func() { res <- Drain(ctx, in) }()
	in <- 1
	in <- 2
	cancel()
	select {
	case got := <-res:
		if !reflect.DeepEqual(got, []int{1, 2}) {
			t.Errorf("Drain = %v; want [1 2]", got)
		}
	case <-time.After(3 * time.Second):
		t.Fatal("Drain did not return after cancel")
	}
}
