package main

import (
	"fmt"
	"sort"
)

type Store struct {
	data map[string][]byte
}

func NewStore() *Store {
	return &Store{data: map[string][]byte{}}
}

func clone(b []byte) []byte {
	c := make([]byte, len(b))
	copy(c, b)
	return c
}

func (s *Store) Put(key string, value []byte) {
	s.data[key] = clone(value)
}

func (s *Store) Get(key string) ([]byte, bool) {
	v, ok := s.data[key]
	if !ok {
		return nil, false
	}
	return clone(v), true
}

func (s *Store) Delete(key string) bool {
	_, ok := s.data[key]
	delete(s.data, key)
	return ok
}

func (s *Store) Len() int { return len(s.data) }

func (s *Store) Keys() []string {
	keys := make([]string, 0, len(s.data))
	for k := range s.data {
		keys = append(keys, k)
	}
	sort.Strings(keys)
	return keys
}

// Try it with Run.
func main() {
	s := NewStore()
	s.Put("b", []byte("two"))
	s.Put("a", []byte("one"))
	v, ok := s.Get("a")
	fmt.Println(string(v), ok, s.Keys())
}
