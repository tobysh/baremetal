package main

import (
	"context"
	"fmt"
)

// Limiter allows at most n concurrent holders.
type Limiter struct {
}

func NewLimiter(n int) *Limiter {
	return nil
}

func (l *Limiter) Acquire() {
}

func (l *Limiter) Release() {
}

func (l *Limiter) TryAcquire() bool {
	return false
}

func (l *Limiter) AcquireCtx(ctx context.Context) error {
	return nil
}

func (l *Limiter) Do(f func()) {
}

func main() {
	l := NewLimiter(1)
	l.Do(func() { fmt.Println("in the section") })
}
