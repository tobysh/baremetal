package main

import (
	"reflect"
	"sync/atomic"
	"testing"
	"time"
)

// within fails the test if fn takes longer than d (a missing close or Wait usually deadlocks).
func within(t *testing.T, d time.Duration, what string, fn func()) {
	t.Helper()
	done := make(chan struct{})
	go func() { fn(); close(done) }()
	select {
	case <-done:
	case <-time.After(d):
		t.Fatalf("%s did not finish within %v (a channel that is never closed, or a missing wg.Wait?)", what, d)
	}
}

func TestProcess_keeps_input_order(t *testing.T) {
	in := make([]int, 200)
	want := make([]int, 200)
	for i := range in {
		in[i] = i
		want[i] = i * 3
	}
	var got []int
	within(t, 3*time.Second, "Process", func() { got = Process(in, 8, func(n int) int { return n * 3 }) })
	if !reflect.DeepEqual(got, want) {
		t.Errorf("Process results are not in input order or have wrong values; got[:5] = %v", got[:5])
	}
}

func TestProcess_edge_cases(t *testing.T) {
	var got []int
	within(t, 2*time.Second, "Process(empty)", func() { got = Process(nil, 4, func(n int) int { return n }) })
	if got == nil || len(got) != 0 {
		t.Errorf("Process of empty input = %#v; want an empty, non-nil slice", got)
	}
	within(t, 2*time.Second, "Process(workers=0)", func() { got = Process([]int{1, 2, 3}, 0, func(n int) int { return n + 1 }) })
	if !reflect.DeepEqual(got, []int{2, 3, 4}) {
		t.Errorf("Process with 0 workers = %v; want [2 3 4] (treat as 1 worker)", got)
	}
	within(t, 2*time.Second, "Process(more workers than inputs)", func() { got = Process([]int{5}, 20, func(n int) int { return -n }) })
	if !reflect.DeepEqual(got, []int{-5}) {
		t.Errorf("Process with more workers than inputs = %v; want [-5]", got)
	}
}

func TestProcess_really_runs_in_parallel_but_not_more_than_workers(t *testing.T) {
	var running, peak int32
	f := func(n int) int {
		cur := atomic.AddInt32(&running, 1)
		for {
			p := atomic.LoadInt32(&peak)
			if cur <= p || atomic.CompareAndSwapInt32(&peak, p, cur) {
				break
			}
		}
		time.Sleep(20 * time.Millisecond)
		atomic.AddInt32(&running, -1)
		return n
	}
	in := make([]int, 16)
	within(t, 5*time.Second, "Process", func() { Process(in, 4, f) })
	if p := atomic.LoadInt32(&peak); p < 2 {
		t.Errorf("at most %d calls ran at once; with 4 workers they should overlap", p)
	} else if p > 4 {
		t.Errorf("%d calls ran at once; the pool must never exceed its 4 workers", p)
	}
}

func TestGenerate_yields_then_closes(t *testing.T) {
	ch := Generate(4, 5, 6)
	var got []int
	within(t, 2*time.Second, "reading Generate", func() {
		for v := range ch {
			got = append(got, v)
		}
	})
	if !reflect.DeepEqual(got, []int{4, 5, 6}) {
		t.Errorf("Generate(4, 5, 6) yielded %v", got)
	}
}

func TestSquare_and_Collect_pipeline(t *testing.T) {
	var got []int
	within(t, 2*time.Second, "pipeline", func() { got = Collect(Square(Square(Generate(1, 2, 3)))) })
	if !reflect.DeepEqual(got, []int{1, 16, 81}) {
		t.Errorf("Collect(Square(Square(Generate(1, 2, 3)))) = %v; want [1 16 81]", got)
	}
}

func TestCollect_of_nothing(t *testing.T) {
	var got []int
	within(t, 2*time.Second, "Collect(Generate())", func() { got = Collect(Generate()) })
	if got == nil || len(got) != 0 {
		t.Errorf("Collect of an empty stream = %#v; want an empty, non-nil slice", got)
	}
}
