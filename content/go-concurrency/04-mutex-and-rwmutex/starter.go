package main

import "fmt"

// Counter is safe for concurrent use. Add your fields.
type Counter struct {
}

func (c *Counter) Inc() {
}

func (c *Counter) Value() int {
	return 0
}

// Registry maps strings to ints and is safe for concurrent use.
type Registry struct {
}

func NewRegistry() *Registry {
	return nil
}

func (r *Registry) Set(key string, v int) {
}

func (r *Registry) Get(key string) (int, bool) {
	return 0, false
}

func (r *Registry) Incr(key string) int {
	return 0
}

func (r *Registry) Len() int {
	return 0
}

func main() {
	var c Counter
	c.Inc()
	fmt.Println(c.Value())
}
