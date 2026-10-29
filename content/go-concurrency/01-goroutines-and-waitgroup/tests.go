package main

import (
	"reflect"
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
		t.Fatalf("%s did not finish within %v (a missing wg.Wait, or tasks run one at a time?)", what, d)
	}
}

func TestRunAll_runs_tasks_concurrently(t *testing.T) {
	const n = 6
	var started, finished, timedOut int32
	ready := make(chan struct{})
	task := func() {
		if atomic.AddInt32(&started, 1) == n {
			close(ready)
		}
		select {
		case <-ready: // every task must be running at the same time to get here
		case <-time.After(2 * time.Second):
			atomic.AddInt32(&timedOut, 1)
		}
		atomic.AddInt32(&finished, 1)
	}
	tasks := make([]func(), n)
	for i := range tasks {
		tasks[i] = task
	}
	within(t, 6*time.Second, "RunAll", func() { RunAll(tasks...) })
	if atomic.LoadInt32(&timedOut) > 0 {
		t.Errorf("tasks were not all running at once; start every task with its own goroutine before waiting")
	}
	if f := atomic.LoadInt32(&finished); f != n {
		t.Errorf("only %d of %d tasks had finished when RunAll returned; did you wg.Wait()?", f, n)
	}
}

func TestRunAll_waits_for_slow_task(t *testing.T) {
	var done int32
	within(t, 3*time.Second, "RunAll", func() {
		RunAll(
			func() { time.Sleep(150 * time.Millisecond); atomic.StoreInt32(&done, 1) },
			func() {},
		)
	})
	if atomic.LoadInt32(&done) != 1 {
		t.Errorf("RunAll returned before the slow task finished")
	}
}

func TestRunAll_no_tasks(t *testing.T) {
	within(t, time.Second, "RunAll()", func() { RunAll() })
}

func TestSquares(t *testing.T) {
	var got []int
	within(t, 3*time.Second, "Squares", func() { got = Squares(8) })
	if want := []int{0, 1, 4, 9, 16, 25, 36, 49}; !reflect.DeepEqual(got, want) {
		t.Errorf("Squares(8) = %v; want %v", got, want)
	}
	within(t, 3*time.Second, "Squares(500)", func() { got = Squares(500) })
	if len(got) != 500 || got[499] != 499*499 || got[250] != 250*250 {
		t.Errorf("Squares(500) has wrong length or values (len %d)", len(got))
	}
}

func TestSquares_empty(t *testing.T) {
	for _, n := range []int{0, -3} {
		got := Squares(n)
		if got == nil || len(got) != 0 {
			t.Errorf("Squares(%d) = %#v; want an empty, non-nil slice", n, got)
		}
	}
}
