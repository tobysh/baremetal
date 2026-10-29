package main

import (
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
		t.Fatalf("%s did not finish within %v (is there a select with no default or timeout?)", what, d)
	}
}

func TestTrySend(t *testing.T) {
	ch := make(chan int, 2)
	var r1, r2, r3 bool
	within(t, time.Second, "TrySend", func() {
		r1 = TrySend(ch, 1)
		r2 = TrySend(ch, 2)
		r3 = TrySend(ch, 3) // buffer full, nobody reading
	})
	if !r1 || !r2 {
		t.Errorf("TrySend into a channel with room returned %v, %v; want true, true", r1, r2)
	}
	if r3 {
		t.Errorf("TrySend into a full channel returned true; want false (and no waiting)")
	}
	if len(ch) != 2 || <-ch != 1 || <-ch != 2 {
		t.Errorf("the channel should hold exactly the first two values 1, 2")
	}
	unbuf := make(chan int)
	within(t, time.Second, "TrySend(unbuffered, no receiver)", func() { r1 = TrySend(unbuf, 5) })
	if r1 {
		t.Errorf("TrySend on an unbuffered channel with no receiver returned true")
	}
}

func TestTrySend_with_waiting_receiver(t *testing.T) {
	ch := make(chan int)
	got := make(chan int, 1)
	go func() { got <- <-ch }()
	ok := false
	deadline := time.Now().Add(2 * time.Second)
	for !ok && time.Now().Before(deadline) {
		ok = TrySend(ch, 9)
		time.Sleep(time.Millisecond)
	}
	if !ok {
		t.Fatalf("TrySend never succeeded although a receiver was waiting")
	}
	select {
	case v := <-got:
		if v != 9 {
			t.Errorf("receiver got %d; want 9", v)
		}
	case <-time.After(2 * time.Second):
		t.Errorf("receiver never got the value")
	}
}

func TestTryRecv(t *testing.T) {
	ch := make(chan int, 2)
	var v int
	var ok bool
	within(t, time.Second, "TryRecv on empty", func() { v, ok = TryRecv(ch) })
	if ok || v != 0 {
		t.Errorf("TryRecv on an empty channel = %d, %v; want 0, false", v, ok)
	}
	ch <- 5
	within(t, time.Second, "TryRecv with a value", func() { v, ok = TryRecv(ch) })
	if !ok || v != 5 {
		t.Errorf("TryRecv with a queued 5 = %d, %v; want 5, true", v, ok)
	}
	close(ch)
	within(t, time.Second, "TryRecv on closed", func() { v, ok = TryRecv(ch) })
	if ok || v != 0 {
		t.Errorf("TryRecv on a closed channel = %d, %v; want 0, false", v, ok)
	}
}

func TestRecvTimeout_value_arrives(t *testing.T) {
	ch := make(chan int)
	go func() {
		time.Sleep(30 * time.Millisecond)
		ch <- 77
	}()
	var v int
	var ok bool
	within(t, 3*time.Second, "RecvTimeout", func() { v, ok = RecvTimeout(ch, 2*time.Second) })
	if !ok || v != 77 {
		t.Errorf("RecvTimeout = %d, %v; want 77, true (the value comes well before the deadline)", v, ok)
	}
}

func TestRecvTimeout_times_out(t *testing.T) {
	ch := make(chan int)
	var v int
	var ok bool
	start := time.Now()
	within(t, 3*time.Second, "RecvTimeout", func() { v, ok = RecvTimeout(ch, 80*time.Millisecond) })
	if ok || v != 0 {
		t.Errorf("RecvTimeout on a silent channel = %d, %v; want 0, false", v, ok)
	}
	if el := time.Since(start); el < 60*time.Millisecond {
		t.Errorf("RecvTimeout gave up after only %v; it should wait the full duration", el)
	}
}

func TestFirstOf(t *testing.T) {
	for _, fromA := range []bool{true, false} {
		a := make(chan string)
		b := make(chan string)
		go func() {
			if fromA {
				time.Sleep(20 * time.Millisecond)
				a <- "from a"
			} else {
				time.Sleep(20 * time.Millisecond)
				b <- "from b"
			}
		}()
		var got string
		within(t, 3*time.Second, "FirstOf", func() { got = FirstOf(a, b) })
		want := "from b"
		if fromA {
			want = "from a"
		}
		if got != want {
			t.Errorf("FirstOf = %q; want %q", got, want)
		}
	}
}
