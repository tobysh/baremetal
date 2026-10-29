package main

import (
	"fmt"
	"sync"
	"testing"
	"time"
)

var t0 = time.Date(2030, 5, 6, 7, 8, 9, 0, time.UTC)

func TestFakeClock(t *testing.T) {
	c := NewFakeClock(t0)
	if !c.Now().Equal(t0) {
		t.Fatalf("Now = %v; want %v", c.Now(), t0)
	}
	c.Advance(90 * time.Minute)
	if want := t0.Add(90 * time.Minute); !c.Now().Equal(want) {
		t.Errorf("after Advance Now = %v; want %v", c.Now(), want)
	}
	c.Advance(0)
	c.Advance(time.Nanosecond)
	if want := t0.Add(90*time.Minute + time.Nanosecond); !c.Now().Equal(want) {
		t.Errorf("Now = %v; want %v", c.Now(), want)
	}
	var _ Clock = c // *FakeClock must satisfy Clock
}

func TestFakeClock_does_not_move_by_itself(t *testing.T) {
	c := NewFakeClock(t0)
	a := c.Now()
	for i := 0; i < 1000; i++ {
	}
	if !c.Now().Equal(a) {
		t.Error("the fake clock moved without Advance")
	}
}

func TestFakeClock_concurrent_advance(t *testing.T) {
	c := NewFakeClock(t0)
	var wg sync.WaitGroup
	for i := 0; i < 100; i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			c.Advance(time.Second)
			c.Now()
		}()
	}
	wg.Wait()
	if want := t0.Add(100 * time.Second); !c.Now().Equal(want) {
		t.Errorf("Now = %v; want %v", c.Now(), want)
	}
}

func TestSystemClock(t *testing.T) {
	var c Clock = SystemClock{}
	before := time.Now()
	got := c.Now()
	if got.Before(before) || got.After(time.Now().Add(time.Second)) {
		t.Errorf("SystemClock.Now() = %v, which is not the current time", got)
	}
}

func TestTTLCache_basic_expiry_boundaries(t *testing.T) {
	clock := NewFakeClock(t0)
	cache := NewTTLCache(clock, 10*time.Second)
	cache.Set("k", "v")
	steps := []struct {
		advance time.Duration
		want    bool
	}{
		{0, true},
		{9 * time.Second, true},
		{999 * time.Millisecond, true},
		{time.Millisecond, false}, // exactly 10s after Set
		{time.Hour, false},
	}
	for _, s := range steps {
		clock.Advance(s.advance)
		v, ok := cache.Get("k")
		if ok != s.want {
			t.Fatalf("after +%v (total %v): ok = %v; want %v", s.advance, clock.Now().Sub(t0), ok, s.want)
		}
		if ok && v != "v" {
			t.Fatalf("value = %q", v)
		}
	}
}

func TestTTLCache_set_again_refreshes(t *testing.T) {
	clock := NewFakeClock(t0)
	cache := NewTTLCache(clock, time.Minute)
	cache.Set("a", "1")
	clock.Advance(50 * time.Second)
	cache.Set("a", "2")
	clock.Advance(50 * time.Second) // 100s since first Set, 50s since the second
	if v, ok := cache.Get("a"); !ok || v != "2" {
		t.Errorf("Get = %q, %v; want 2, true (Set restarts the TTL)", v, ok)
	}
	clock.Advance(10 * time.Second)
	if _, ok := cache.Get("a"); ok {
		t.Error("entry should have expired 60s after the latest Set")
	}
}

func TestTTLCache_keys_expire_independently_and_len(t *testing.T) {
	clock := NewFakeClock(t0)
	cache := NewTTLCache(clock, time.Minute)
	if cache.Len() != 0 {
		t.Errorf("empty Len = %d", cache.Len())
	}
	cache.Set("old", "1")
	clock.Advance(40 * time.Second)
	cache.Set("new", "2")
	if cache.Len() != 2 {
		t.Errorf("Len = %d; want 2", cache.Len())
	}
	clock.Advance(20 * time.Second) // old is now 60s, new 20s
	if cache.Len() != 1 {
		t.Errorf("Len = %d; want 1 (old expired)", cache.Len())
	}
	if _, ok := cache.Get("old"); ok {
		t.Error("old should be gone")
	}
	if v, ok := cache.Get("new"); !ok || v != "2" {
		t.Errorf("new = %q, %v", v, ok)
	}
	clock.Advance(time.Hour)
	if cache.Len() != 0 {
		t.Errorf("Len after everything expired = %d", cache.Len())
	}
	if _, ok := cache.Get("never-set"); ok {
		t.Error("unknown key reported ok")
	}
}

func TestTTLCache_different_ttls(t *testing.T) {
	for _, ttl := range []time.Duration{time.Second, 5 * time.Minute, 24 * time.Hour} {
		clock := NewFakeClock(t0)
		cache := NewTTLCache(clock, ttl)
		cache.Set("k", "v")
		clock.Advance(ttl - time.Nanosecond)
		if _, ok := cache.Get("k"); !ok {
			t.Errorf("ttl %v: expired 1ns early", ttl)
		}
		clock.Advance(time.Nanosecond)
		if _, ok := cache.Get("k"); ok {
			t.Errorf("ttl %v: still present at exactly the TTL", ttl)
		}
	}
}

func TestTTLCache_nil_clock_uses_the_system_clock(t *testing.T) {
	cache := NewTTLCache(nil, time.Hour)
	cache.Set("x", "y")
	if v, ok := cache.Get("x"); !ok || v != "y" {
		t.Errorf("Get = %q, %v", v, ok)
	}
}

func TestTTLCache_concurrent_use(t *testing.T) {
	clock := NewFakeClock(t0)
	cache := NewTTLCache(clock, time.Hour)
	var wg sync.WaitGroup
	for i := 0; i < 50; i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			k := fmt.Sprint("k", i)
			cache.Set(k, k)
			cache.Get(k)
			cache.Len()
		}()
	}
	wg.Wait()
	if cache.Len() != 50 {
		t.Errorf("Len = %d; want 50", cache.Len())
	}
}
