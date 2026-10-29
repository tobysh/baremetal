package main

import (
	"fmt"
)

type version struct {
	seq     uint64
	value   string
	deleted bool
}

type Store struct {
	seq      uint64
	versions map[string][]version
	snaps    map[*Snapshot]struct{}
}

type Snapshot struct {
	store *Store
	seq   uint64
}

func NewStore() *Store {
	return &Store{versions: map[string][]version{}, snaps: map[*Snapshot]struct{}{}}
}

func (s *Store) Put(key, value string) {}
func (s *Store) Delete(key string)     {}

func (s *Store) Get(key string) (string, bool) { return "", false }

func (s *Store) Snapshot() *Snapshot {
	return &Snapshot{store: s, seq: s.seq}
}

func (sn *Snapshot) Get(key string) (string, bool) { return "", false }

func (sn *Snapshot) Keys() []string { return nil }

func (sn *Snapshot) Close() {}

func (s *Store) Versions() int { return 0 }

func (s *Store) Prune() {}

// Try it with Run.
func main() {
	s := NewStore()
	s.Put("k", "a")
	snap := s.Snapshot()
	s.Put("k", "b")
	fmt.Println(snap.Get("k"))
	fmt.Println(s.Get("k"))
	snap.Close()
	s.Prune()
	fmt.Println(s.Versions())
}
