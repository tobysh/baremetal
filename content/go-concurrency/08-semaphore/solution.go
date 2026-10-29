package main

import (
	"context"
	"fmt"
)

type Limiter struct {
	slots chan struct{}
}

func NewLimiter(n int) *Limiter {
	if n < 1 {
		n = 1
	}
	return &Limiter{slots: make(chan struct{}, n)}
}

func (l *Limiter) Acquire() { l.slots <- struct{}{} }

func (l *Limiter) Release() { <-l.slots }

func (l *Limiter) TryAcquire() bool {
	select {
	case l.slots <- struct{}{}:
		return true
	default:
		return false
	}
}

func (l *Limiter) AcquireCtx(ctx context.Context) error {
	select {
	case l.slots <- struct{}{}:
		return nil
	case <-ctx.Done():
		return ctx.Err()
	}
}

func (l *Limiter) Do(f func()) {
	l.Acquire()
	defer l.Release()
	f()
}

func main() {
	l := NewLimiter(1)
	l.Do(func() { fmt.Println("in the section") })
}
