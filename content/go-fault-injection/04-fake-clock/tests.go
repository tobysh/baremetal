package main

import (
	"math"
	"math/rand"
	"testing"
)

type fakeClock struct{ now int64 }

func (c *fakeClock) NowMs() int64    { return c.now }
func (c *fakeClock) Advance(d int64) { c.now += d }
func (c *fakeClock) Set(t int64)     { c.now = t }

// refModel simulates refills one interval at a time instead of dividing.
type refModel struct {
	cap, tokens int
	interval    int64
	last        int64
}

func (m *refModel) sync(now int64) {
	if now < m.last {
		m.last = now
		return
	}
	for m.tokens < m.cap && now-m.last >= m.interval {
		m.tokens++
		m.last += m.interval
	}
	if m.tokens == m.cap {
		m.last = now
	}
}

func (m *refModel) allow(now int64) bool {
	m.sync(now)
	if m.tokens > 0 {
		m.tokens--
		return true
	}
	return false
}

func TestLimiter_basic_refill(t *testing.T) {
	c := &fakeClock{now: 5000}
	l := NewLimiter(c, 3, 1000)
	for i := 0; i < 3; i++ {
		if !l.Allow() {
			t.Fatalf("starts full: call %d denied", i)
		}
	}
	if l.Allow() {
		t.Fatal("fourth call should be denied")
	}
	c.Advance(999)
	if l.Allow() {
		t.Fatal("999 ms is not a full interval")
	}
	c.Advance(1)
	if !l.Allow() || l.Allow() {
		t.Fatal("one interval gives exactly one token")
	}
	c.Advance(2500)
	if l.Tokens() != 2 {
		t.Fatalf("2500 ms gives 2 tokens, got %d", l.Tokens())
	}
	c.Advance(500)
	if l.Tokens() != 3 {
		t.Fatalf("the kept half interval plus 500 ms makes a third token, got %d", l.Tokens())
	}
}

func TestLimiter_backward_jump_mints_nothing_and_rebases(t *testing.T) {
	c := &fakeClock{now: 10_000_000}
	l := NewLimiter(c, 2, 1000)
	l.Allow()
	l.Allow()
	c.Set(10_000_000 - 3_600_000)
	if l.Allow() {
		t.Fatal("an hour backwards must not create tokens")
	}
	if got := l.Tokens(); got != 0 {
		t.Fatalf("tokens after backward jump = %d, want 0", got)
	}
	c.Advance(1000)
	if got := l.Tokens(); got != 1 {
		t.Fatalf("one interval after the rebase: tokens = %d, want exactly 1 (not an hour of waiting)", got)
	}
}

func TestLimiter_giant_forward_jump_is_capped_without_overflow(t *testing.T) {
	c := &fakeClock{}
	l := NewLimiter(c, 5, 1)
	for i := 0; i < 5; i++ {
		l.Allow()
	}
	c.Advance(math.MaxInt64 / 2)
	if got := l.Tokens(); got != 5 {
		t.Fatalf("tokens after a huge leap = %d, want capacity 5", got)
	}
	c.Set(0)
	for i := 0; i < 5; i++ {
		l.Allow()
	}
	c.Set(math.MaxInt64 / 2)
	if got := l.Tokens(); got != 5 {
		t.Fatalf("second leap: tokens = %d, want 5", got)
	}
}

func TestLimiter_idle_time_at_capacity_is_not_credit(t *testing.T) {
	c := &fakeClock{}
	l := NewLimiter(c, 2, 1000)
	c.Advance(900)
	l.Allow() // full bucket: spends one
	c.Advance(900)
	if got := l.Tokens(); got != 1 {
		t.Fatalf("tokens = %d, want 1: the 900 ms spent while full must not count toward the next token", got)
	}
	c.Advance(100)
	if got := l.Tokens(); got != 2 {
		t.Fatalf("tokens = %d, want 2: the token spent at t=900 is back at t=1900", got)
	}
}

func TestLimiter_random_scripts_match_reference(t *testing.T) {
	for seed := int64(0); seed < 300; seed++ {
		r := rand.New(rand.NewSource(seed))
		capacity := 1 + r.Intn(6)
		interval := int64(1 + r.Intn(500))
		start := int64(r.Intn(100000))
		c := &fakeClock{now: start}
		l := NewLimiter(c, capacity, interval)
		m := &refModel{cap: capacity, tokens: capacity, interval: interval, last: start}
		for step := 0; step < 80; step++ {
			switch r.Intn(6) {
			case 0:
				c.Advance(int64(r.Intn(int(interval) * 3)))
			case 1:
				c.Advance(interval)
			case 2:
				c.Set(c.now - int64(r.Intn(100000)))
			case 3:
				if r.Intn(10) == 0 {
					c.Set(math.MaxInt64/4 + int64(r.Intn(1000)))
				}
			}
			if r.Intn(3) == 0 {
				m.sync(c.now)
				if got := l.Tokens(); got != m.tokens {
					t.Fatalf("seed %d step %d: Tokens() = %d, model says %d (now=%d)", seed, step, got, m.tokens, c.now)
				}
				continue
			}
			want := m.allow(c.now)
			if got := l.Allow(); got != want {
				t.Fatalf("seed %d step %d: Allow() = %v, model says %v (now=%d, tokens before=%d)", seed, step, got, want, c.now, m.tokens)
			}
		}
	}
}
