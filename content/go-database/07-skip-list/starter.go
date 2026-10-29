package main

import (
	"fmt"
	"math/rand"
)

const maxHeight = 16

type node struct {
	key   string
	value []byte
	next  []*node
}

type SkipList struct {
	head   *node
	height int
	n      int
	rng    *rand.Rand
}

func NewSkipList(seed int64) *SkipList {
	return &SkipList{
		head:   &node{next: make([]*node, maxHeight)},
		height: 1,
		rng:    rand.New(rand.NewSource(seed)),
	}
}

func (s *SkipList) randomHeight() int {
	return 1
}

func (s *SkipList) Put(key string, value []byte) {
}

func (s *SkipList) Get(key string) ([]byte, bool) {
	return nil, false
}

func (s *SkipList) Delete(key string) bool {
	return false
}

func (s *SkipList) Len() int { return 0 }

func (s *SkipList) Seek(key string) (string, []byte, bool) {
	return "", nil, false
}

func (s *SkipList) Keys() []string {
	return nil
}

func (s *SkipList) Height() int { return 1 }

// Try it with Run.
func main() {
	s := NewSkipList(1)
	for _, k := range []string{"pear", "apple", "fig", "kiwi"} {
		s.Put(k, []byte(k))
	}
	k, _, _ := s.Seek("g")
	fmt.Println(s.Keys(), k, s.Height())
}
