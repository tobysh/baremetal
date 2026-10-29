package main

import (
	"fmt"
	"sync"
)

type Counter struct {
	mu sync.Mutex // guards n
	n  int
}

func (c *Counter) Inc() {
	c.mu.Lock()
	defer c.mu.Unlock()
	c.n++
}

func (c *Counter) Value() int {
	c.mu.Lock()
	defer c.mu.Unlock()
	return c.n
}

type Registry struct {
	mu sync.RWMutex // guards m
	m  map[string]int
}

func NewRegistry() *Registry {
	return &Registry{m: make(map[string]int)}
}

func (r *Registry) Set(key string, v int) {
	r.mu.Lock()
	defer r.mu.Unlock()
	r.m[key] = v
}

func (r *Registry) Get(key string) (int, bool) {
	r.mu.RLock()
	defer r.mu.RUnlock()
	v, ok := r.m[key]
	return v, ok
}

func (r *Registry) Incr(key string) int {
	r.mu.Lock()
	defer r.mu.Unlock()
	r.m[key]++
	return r.m[key]
}

func (r *Registry) Len() int {
	r.mu.RLock()
	defer r.mu.RUnlock()
	return len(r.m)
}

func main() {
	var c Counter
	c.Inc()
	fmt.Println(c.Value())
}
