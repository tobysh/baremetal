package main

// SafeCounter counts keys and is safe for concurrent use.
type SafeCounter struct {
}

func NewSafeCounter() *SafeCounter {
	return &SafeCounter{}
}

// Inc adds one to the count for key.
func (c *SafeCounter) Inc(key string) {
}

// Value returns the count for key (0 if unseen).
func (c *SafeCounter) Value(key string) int {
	return 0
}

// ParallelSum sums nums using the given number of goroutines.
func ParallelSum(nums []int, workers int) int {
	return 0
}
