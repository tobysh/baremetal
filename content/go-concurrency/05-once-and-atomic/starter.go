package main

import "fmt"

// Lazy computes a value once, on first Get.
type Lazy struct {
}

func (l *Lazy) Get(init func() int) int {
	return 0
}

// Stats collects measurements using atomics only.
type Stats struct {
}

func (s *Stats) Record(v int64) {
}

func (s *Stats) Count() int64 {
	return 0
}

func (s *Stats) Total() int64 {
	return 0
}

func (s *Stats) Max() int64 {
	return 0
}

func main() {
	var l Lazy
	fmt.Println(l.Get(func() int { return 42 }), l.Get(func() int { return 7 }))
}
