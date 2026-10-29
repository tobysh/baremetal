package main

import (
	"reflect"
	"sort"
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
		t.Fatalf("%s did not finish within %v (an output that is never closed, or closed too early?)", what, d)
	}
}

func source(nums ...int) <-chan int {
	out := make(chan int)
	go func() {
		defer close(out)
		for _, n := range nums {
			out <- n
		}
	}()
	return out
}

func drain(c <-chan int) []int {
	var r []int
	for v := range c {
		r = append(r, v)
	}
	return r
}

func TestMerge_all_values_and_per_input_order(t *testing.T) {
	var got []int
	within(t, 3*time.Second, "Merge", func() {
		got = drain(Merge(source(1, 2, 3, 4), source(100, 200, 300), source(-1, -2)))
	})
	sorted := append([]int(nil), got...)
	sort.Ints(sorted)
	if want := []int{-2, -1, 1, 2, 3, 4, 100, 200, 300}; !reflect.DeepEqual(sorted, want) {
		t.Fatalf("Merge lost or invented values; got %v", got)
	}
	// relative order inside each input must be preserved
	pos := map[int]int{}
	for i, v := range got {
		pos[v] = i
	}
	if !(pos[1] < pos[2] && pos[2] < pos[3] && pos[3] < pos[4]) || !(pos[100] < pos[200] && pos[200] < pos[300]) || !(pos[-1] < pos[-2]) {
		t.Errorf("values from one input arrived out of order: %v", got)
	}
}

func TestMerge_closes_only_after_slowest_input(t *testing.T) {
	slow := make(chan int)
	go func() {
		time.Sleep(150 * time.Millisecond)
		slow <- 9
		close(slow)
	}()
	var got []int
	within(t, 3*time.Second, "Merge with a slow input", func() { got = drain(Merge(source(1, 2), slow)) })
	sort.Ints(got)
	if want := []int{1, 2, 9}; !reflect.DeepEqual(got, want) {
		t.Errorf("Merge = %v; want %v (the output must stay open until every input is closed)", got, want)
	}
}

func TestMerge_no_inputs_and_empty_inputs(t *testing.T) {
	var got []int
	within(t, 2*time.Second, "Merge()", func() { got = drain(Merge()) })
	if len(got) != 0 {
		t.Errorf("Merge() yielded %v", got)
	}
	within(t, 2*time.Second, "Merge of empty inputs", func() { got = drain(Merge(source(), source())) })
	if len(got) != 0 {
		t.Errorf("Merge of empty inputs yielded %v", got)
	}
}

func TestMerge_returns_immediately(t *testing.T) {
	block := make(chan int)
	within(t, time.Second, "Merge must not wait for its inputs", func() { Merge(block) })
	close(block)
}

func TestFanOut_shape(t *testing.T) {
	outs := FanOut(source(1, 2, 3), 4, func(n int) int { return n })
	if len(outs) != 4 {
		t.Fatalf("FanOut with 4 workers returned %d channels; want 4", len(outs))
	}
	for i := range outs {
		_ = i
	}
	one := FanOut(source(5), 0, func(n int) int { return n })
	if len(one) != 1 {
		t.Errorf("FanOut with 0 workers returned %d channels; want 1", len(one))
	}
	drain(Merge(outs...))
	drain(Merge(one...))
}

func TestFanOut_with_Merge_computes_everything(t *testing.T) {
	in := make([]int, 100)
	want := make([]int, 100)
	for i := range in {
		in[i] = i
		want[i] = i*i + 1
	}
	var got []int
	within(t, 4*time.Second, "FanOut + Merge", func() {
		got = drain(Merge(FanOut(source(in...), 5, func(n int) int { return n*n + 1 })...))
	})
	sort.Ints(got)
	sort.Ints(want)
	if !reflect.DeepEqual(got, want) {
		t.Errorf("each input must be processed exactly once; got %d values, first few %v", len(got), got[:min(5, len(got))])
	}
}

func TestFanOut_runs_in_parallel(t *testing.T) {
	in := make([]int, 12)
	started := make(chan struct{}, len(in))
	release := make(chan struct{})
	finished := make(chan struct{})
	go func() {
		drain(Merge(FanOut(source(in...), 4, func(n int) int {
			started <- struct{}{}
			<-release
			return n
		})...))
		close(finished)
	}()
	func() {
		defer close(release)
		within(t, 3*time.Second, "four workers overlap before release", func() {
			for i := 0; i < 4; i++ {
				<-started
			}
		})
	}()
	within(t, 3*time.Second, "FanOut finishes after release", func() { <-finished })
}

func TestFanOut_closes_outputs_when_input_closes(t *testing.T) {
	outs := FanOut(source(), 3, func(n int) int { return n })
	for i, o := range outs {
		select {
		case _, ok := <-o:
			if ok {
				t.Errorf("output %d produced a value from an empty input", i)
			}
		case <-time.After(2 * time.Second):
			t.Errorf("output %d was not closed after the input closed", i)
		}
	}
}
