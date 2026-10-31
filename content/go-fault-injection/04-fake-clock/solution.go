package main

import "fmt"

type Clock interface{ NowMs() int64 }

type Limiter struct {
	clock    Clock
	capacity int
	interval int64
	tokens   int
	last     int64
}

func NewLimiter(c Clock, capacity int, intervalMs int64) *Limiter {
	return &Limiter{clock: c, capacity: capacity, interval: intervalMs, tokens: capacity, last: c.NowMs()}
}

func (l *Limiter) refill() {
	now := l.clock.NowMs()
	switch {
	case now < l.last:
		l.last = now
	case l.tokens == l.capacity:
		l.last = now
	default:
		n := (now - l.last) / l.interval
		if n >= int64(l.capacity-l.tokens) {
			l.tokens = l.capacity
			l.last = now
		} else {
			l.tokens += int(n)
			l.last += n * l.interval
		}
	}
}

func (l *Limiter) Allow() bool {
	l.refill()
	if l.tokens > 0 {
		l.tokens--
		return true
	}
	return false
}

func (l *Limiter) Tokens() int {
	l.refill()
	return l.tokens
}

type stepClock struct{ now int64 }

func (c *stepClock) NowMs() int64 { return c.now }

func main() {
	c := &stepClock{}
	l := NewLimiter(c, 2, 1000)
	fmt.Println(l.Allow(), l.Allow(), l.Allow())
	c.now = 1000
	fmt.Println(l.Allow(), l.Allow())
}
