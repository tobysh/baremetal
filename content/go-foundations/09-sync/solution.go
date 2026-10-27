package main

import "sync"

// SafeCounter counts keys and is safe for concurrent use.
type SafeCounter struct {
	mu     sync.Mutex
	counts map[string]int
}

func NewSafeCounter() *SafeCounter {
	return &SafeCounter{counts: make(map[string]int)}
}

// Inc adds one to the count for key.
func (c *SafeCounter) Inc(key string) {
	c.mu.Lock()
	defer c.mu.Unlock()
	c.counts[key]++
}

// Value returns the count for key (0 if unseen).
func (c *SafeCounter) Value(key string) int {
	c.mu.Lock()
	defer c.mu.Unlock()
	return c.counts[key]
}

// ParallelSum sums nums using the given number of goroutines.
func ParallelSum(nums []int, workers int) int {
	if workers < 1 {
		workers = 1
	}
	size := (len(nums) + workers - 1) / workers
	partial := make([]int, workers)
	var wg sync.WaitGroup
	for i := 0; i < workers; i++ {
		lo := i * size
		if lo >= len(nums) {
			break
		}
		hi := lo + size
		if hi > len(nums) {
			hi = len(nums)
		}
		wg.Add(1)
		go func() {
			defer wg.Done()
			for _, n := range nums[lo:hi] {
				partial[i] += n
			}
		}()
	}
	wg.Wait()
	total := 0
	for _, p := range partial {
		total += p
	}
	return total
}
