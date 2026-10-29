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
	h := 1
	for h < maxHeight && s.rng.Intn(2) == 0 {
		h++
	}
	return h
}

// find fills update with the predecessor of key on every level and
// returns the first node whose key is >= key.
func (s *SkipList) find(key string, update *[maxHeight]*node) *node {
	x := s.head
	for lvl := s.height - 1; lvl >= 0; lvl-- {
		for x.next[lvl] != nil && x.next[lvl].key < key {
			x = x.next[lvl]
		}
		update[lvl] = x
	}
	return x.next[0]
}

func (s *SkipList) Put(key string, value []byte) {
	var update [maxHeight]*node
	v := append([]byte(nil), value...)
	if c := s.find(key, &update); c != nil && c.key == key {
		c.value = v
		return
	}
	h := s.randomHeight()
	if h > s.height {
		for lvl := s.height; lvl < h; lvl++ {
			update[lvl] = s.head
		}
		s.height = h
	}
	n := &node{key: key, value: v, next: make([]*node, h)}
	for lvl := 0; lvl < h; lvl++ {
		n.next[lvl] = update[lvl].next[lvl]
		update[lvl].next[lvl] = n
	}
	s.n++
}

func (s *SkipList) Get(key string) ([]byte, bool) {
	var update [maxHeight]*node
	if c := s.find(key, &update); c != nil && c.key == key {
		return append([]byte(nil), c.value...), true
	}
	return nil, false
}

func (s *SkipList) Delete(key string) bool {
	var update [maxHeight]*node
	c := s.find(key, &update)
	if c == nil || c.key != key {
		return false
	}
	for lvl := 0; lvl < s.height; lvl++ {
		if update[lvl].next[lvl] != c {
			break
		}
		update[lvl].next[lvl] = c.next[lvl]
	}
	for s.height > 1 && s.head.next[s.height-1] == nil {
		s.height--
	}
	s.n--
	return true
}

func (s *SkipList) Len() int { return s.n }

func (s *SkipList) Seek(key string) (string, []byte, bool) {
	var update [maxHeight]*node
	c := s.find(key, &update)
	if c == nil {
		return "", nil, false
	}
	return c.key, append([]byte(nil), c.value...), true
}

func (s *SkipList) Keys() []string {
	keys := make([]string, 0, s.n)
	for x := s.head.next[0]; x != nil; x = x.next[0] {
		keys = append(keys, x.key)
	}
	return keys
}

func (s *SkipList) Height() int { return s.height }

// Try it with Run.
func main() {
	s := NewSkipList(1)
	for _, k := range []string{"pear", "apple", "fig", "kiwi"} {
		s.Put(k, []byte(k))
	}
	k, _, _ := s.Seek("g")
	fmt.Println(s.Keys(), k, s.Height())
}
