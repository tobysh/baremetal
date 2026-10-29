package main

import (
	"fmt"
	"sort"
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

func (s *Store) write(key, value string, deleted bool) {
	s.seq++
	s.versions[key] = append(s.versions[key], version{s.seq, value, deleted})
}

func (s *Store) Put(key, value string) { s.write(key, value, false) }
func (s *Store) Delete(key string)     { s.write(key, "", true) }

// visible returns the newest version with seq <= at.
func visible(vs []version, at uint64) (version, bool) {
	for i := len(vs) - 1; i >= 0; i-- {
		if vs[i].seq <= at {
			return vs[i], true
		}
	}
	return version{}, false
}

func (s *Store) read(key string, at uint64) (string, bool) {
	v, ok := visible(s.versions[key], at)
	if !ok || v.deleted {
		return "", false
	}
	return v.value, true
}

func (s *Store) Get(key string) (string, bool) { return s.read(key, s.seq) }

func (s *Store) Snapshot() *Snapshot {
	snap := &Snapshot{store: s, seq: s.seq}
	s.snaps[snap] = struct{}{}
	return snap
}

func (sn *Snapshot) Get(key string) (string, bool) { return sn.store.read(key, sn.seq) }

func (sn *Snapshot) Keys() []string {
	keys := []string{}
	for k := range sn.store.versions {
		if _, ok := sn.store.read(k, sn.seq); ok {
			keys = append(keys, k)
		}
	}
	sort.Strings(keys)
	return keys
}

func (sn *Snapshot) Close() { delete(sn.store.snaps, sn) }

func (s *Store) Versions() int {
	n := 0
	for _, vs := range s.versions {
		n += len(vs)
	}
	return n
}

func (s *Store) Prune() {
	oldest := s.seq
	for sn := range s.snaps {
		if sn.seq < oldest {
			oldest = sn.seq
		}
	}
	for k, vs := range s.versions {
		i := -1
		for j := range vs {
			if vs[j].seq <= oldest {
				i = j
			}
		}
		if i < 0 {
			continue
		}
		if vs[i].deleted {
			i++
		}
		if i >= len(vs) {
			delete(s.versions, k)
			continue
		}
		s.versions[k] = append([]version(nil), vs[i:]...)
	}
}

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
