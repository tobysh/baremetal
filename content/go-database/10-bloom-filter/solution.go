package main

import (
	"encoding/binary"
	"errors"
	"fmt"
	"hash/fnv"
)

type Bloom struct {
	bits []uint64
	m    uint32
	k    int
}

func NewBloom(n, bitsPerKey int) *Bloom {
	m := n * bitsPerKey
	if m < 64 {
		m = 64
	}
	m = (m + 63) / 64 * 64
	k := int(float64(bitsPerKey)*0.69 + 0.5)
	if k < 1 {
		k = 1
	}
	if k > 30 {
		k = 30
	}
	return &Bloom{bits: make([]uint64, m/64), m: uint32(m), k: k}
}

func (b *Bloom) each(key []byte, fn func(pos uint32) bool) bool {
	h := fnv.New64a()
	h.Write(key)
	sum := h.Sum64()
	h1, h2 := uint32(sum), uint32(sum>>32)|1
	for i := 0; i < b.k; i++ {
		if !fn((h1 + uint32(i)*h2) % b.m) {
			return false
		}
	}
	return true
}

func (b *Bloom) Add(key []byte) {
	b.each(key, func(p uint32) bool {
		b.bits[p/64] |= 1 << (p % 64)
		return true
	})
}

func (b *Bloom) MayContain(key []byte) bool {
	return b.each(key, func(p uint32) bool {
		return b.bits[p/64]&(1<<(p%64)) != 0
	})
}

func (b *Bloom) Marshal() []byte {
	out := []byte{byte(b.k)}
	out = binary.BigEndian.AppendUint32(out, b.m)
	for _, w := range b.bits {
		out = binary.BigEndian.AppendUint64(out, w)
	}
	return out
}

func UnmarshalBloom(data []byte) (*Bloom, error) {
	if len(data) < 5 {
		return nil, errors.New("bloom: too short")
	}
	k := int(data[0])
	m := binary.BigEndian.Uint32(data[1:5])
	if k < 1 || k > 30 || m == 0 {
		return nil, errors.New("bloom: bad parameters")
	}
	words := (uint64(m) + 63) / 64
	if uint64(len(data)) != 5+8*words || uint64(m)%64 != 0 {
		return nil, errors.New("bloom: wrong length")
	}
	b := &Bloom{bits: make([]uint64, words), m: m, k: k}
	for i := range b.bits {
		b.bits[i] = binary.BigEndian.Uint64(data[5+8*i:])
	}
	return b, nil
}

// Try it with Run.
func main() {
	b := NewBloom(100, 10)
	b.Add([]byte("apple"))
	fmt.Println(b.MayContain([]byte("apple")), b.MayContain([]byte("pear")))
}
