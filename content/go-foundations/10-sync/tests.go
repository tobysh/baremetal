package main

import (
	"sync"
	"testing"
	"time"
)

func TestSafeCounter_counts_sequentially(t *testing.T) {
	c := NewSafeCounter()
	c.Inc("a")
	c.Inc("a")
	c.Inc("b")
	if c.Value("a") != 2 || c.Value("b") != 1 || c.Value("zzz") != 0 {
		t.Errorf("a=%d b=%d zzz=%d; want 2, 1, 0", c.Value("a"), c.Value("b"), c.Value("zzz"))
	}
}

func TestSafeCounter_survives_many_goroutines(t *testing.T) {
	c := NewSafeCounter()
	var wg sync.WaitGroup
	for g := 0; g < 50; g++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			for i := 0; i < 200; i++ {
				c.Inc("hits")
				c.Inc("other")
			}
		}()
	}
	wg.Wait()
	if got := c.Value("hits"); got != 10000 {
		t.Errorf("after 50 goroutines x 200 Inc, Value(\"hits\") = %d; want 10000 (lost updates mean a missing lock)", got)
	}
}

func TestParallelSum_small_inputs(t *testing.T) {
	if got := ParallelSum([]int{1, 2, 3, 4, 5}, 2); got != 15 {
		t.Errorf("ParallelSum([1..5], 2) = %d; want 15", got)
	}
	if got := ParallelSum(nil, 4); got != 0 {
		t.Errorf("ParallelSum(nil, 4) = %d; want 0", got)
	}
}

func TestParallelSum_more_workers_than_numbers(t *testing.T) {
	if got := ParallelSum([]int{10, 20}, 8); got != 30 {
		t.Errorf("ParallelSum([10 20], 8) = %d; want 30", got)
	}
}

func TestParallelSum_workers_below_one(t *testing.T) {
	if got := ParallelSum([]int{1, 2, 3}, 0); got != 6 {
		t.Errorf("ParallelSum([1 2 3], 0) = %d; want 6", got)
	}
	if got := ParallelSum([]int{1, 2, 3}, -3); got != 6 {
		t.Errorf("ParallelSum([1 2 3], -3) = %d; want 6", got)
	}
}

func TestParallelSum_large_input_and_uneven_split(t *testing.T) {
	nums := make([]int, 100003)
	want := 0
	for i := range nums {
		nums[i] = i % 7
		want += nums[i]
	}
	done := make(chan int, 1)
	go func() { done <- ParallelSum(nums, 7) }()
	select {
	case got := <-done:
		if got != want {
			t.Errorf("ParallelSum of 100003 numbers with 7 workers = %d; want %d", got, want)
		}
	case <-time.After(3 * time.Second):
		t.Fatalf("ParallelSum did not finish (is wg.Done never called?)")
	}
}
