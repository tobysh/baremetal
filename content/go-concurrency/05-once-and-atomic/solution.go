package main

import (
	"fmt"
	"sync"
	"sync/atomic"
)

type Lazy struct {
	once sync.Once
	val  int
}

func (l *Lazy) Get(init func() int) int {
	l.once.Do(func() { l.val = init() })
	return l.val
}

type Stats struct {
	count atomic.Int64
	total atomic.Int64
	max   atomic.Int64
}

func (s *Stats) Record(v int64) {
	s.count.Add(1)
	s.total.Add(v)
	for {
		cur := s.max.Load()
		if v <= cur || s.max.CompareAndSwap(cur, v) {
			return
		}
	}
}

func (s *Stats) Count() int64 { return s.count.Load() }
func (s *Stats) Total() int64 { return s.total.Load() }
func (s *Stats) Max() int64   { return s.max.Load() }

func main() {
	var l Lazy
	fmt.Println(l.Get(func() int { return 42 }), l.Get(func() int { return 7 }))
}
