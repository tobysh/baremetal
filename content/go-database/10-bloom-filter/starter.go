package main

import (
	"errors"
	"fmt"
)

type Bloom struct {
	bits []uint64
	m    uint32
	k    int
}

func NewBloom(n, bitsPerKey int) *Bloom {
	return &Bloom{}
}

func (b *Bloom) Add(key []byte) {
}

func (b *Bloom) MayContain(key []byte) bool {
	return true
}

func (b *Bloom) Marshal() []byte {
	return nil
}

func UnmarshalBloom(data []byte) (*Bloom, error) {
	return nil, errors.New("not implemented")
}

// Try it with Run.
func main() {
	b := NewBloom(100, 10)
	b.Add([]byte("apple"))
	fmt.Println(b.MayContain([]byte("apple")), b.MayContain([]byte("pear")))
}
