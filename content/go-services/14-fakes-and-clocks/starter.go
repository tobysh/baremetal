package main

import (
	"fmt"
	"sync"
	"time"
)

type Clock interface {
	Now() time.Time
}

type SystemClock struct{}

func (SystemClock) Now() time.Time {
	return time.Time{}
}

type FakeClock struct {
	mu  sync.Mutex
	now time.Time
}

func NewFakeClock(start time.Time) *FakeClock {
	return &FakeClock{}
}

func (c *FakeClock) Now() time.Time {
	return time.Time{}
}

func (c *FakeClock) Advance(d time.Duration) {
}

type TTLCache struct {
	mu sync.Mutex
}

func NewTTLCache(clock Clock, ttl time.Duration) *TTLCache {
	return &TTLCache{}
}

func (c *TTLCache) Set(key, value string) {
}

func (c *TTLCache) Get(key string) (string, bool) {
	return "", false
}

func (c *TTLCache) Len() int {
	return 0
}

// Try it with Run.
func main() {
	clock := NewFakeClock(time.Date(2026, 1, 1, 0, 0, 0, 0, time.UTC))
	cache := NewTTLCache(clock, time.Minute)
	cache.Set("a", "1")
	clock.Advance(59 * time.Second)
	fmt.Println(cache.Get("a"))
	clock.Advance(time.Second)
	fmt.Println(cache.Get("a"))
}
