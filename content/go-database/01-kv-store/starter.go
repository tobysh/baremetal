package main

import "fmt"

type Store struct {
	data map[string][]byte
}

func NewStore() *Store {
	return &Store{}
}

func (s *Store) Put(key string, value []byte) {
}

func (s *Store) Get(key string) ([]byte, bool) {
	return nil, false
}

func (s *Store) Delete(key string) bool {
	return false
}

func (s *Store) Len() int { return 0 }

func (s *Store) Keys() []string {
	return nil
}

// Try it with Run.
func main() {
	s := NewStore()
	s.Put("b", []byte("two"))
	s.Put("a", []byte("one"))
	v, ok := s.Get("a")
	fmt.Println(string(v), ok, s.Keys())
}
