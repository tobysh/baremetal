package main

import (
	"encoding/binary"
	"errors"
	"fmt"
)

var ErrNoMem = errors.New("out of memory")

type Allocator interface {
	Alloc(n int) ([]byte, error)
	Free(b []byte)
}

type Packed struct {
	data, index []byte
	count       int
}

func Pack(a Allocator, items []string) (*Packed, error) {
	total := 0
	for _, s := range items {
		total += len(s)
	}
	data, err := a.Alloc(total)
	if err != nil {
		return nil, err
	}
	index, err := a.Alloc(4 * len(items))
	if err != nil {
		a.Free(data)
		return nil, err
	}
	scratch, err := a.Alloc(4)
	if err != nil {
		a.Free(index)
		a.Free(data)
		return nil, err
	}
	defer a.Free(scratch)
	pos := 0
	for i, s := range items {
		copy(data[pos:], s)
		pos += len(s)
		binary.BigEndian.PutUint32(scratch, uint32(pos))
		copy(index[4*i:], scratch)
	}
	return &Packed{data, index, len(items)}, nil
}

func (p *Packed) Item(i int) string {
	start := 0
	if i > 0 {
		start = int(binary.BigEndian.Uint32(p.index[4*(i-1):]))
	}
	end := int(binary.BigEndian.Uint32(p.index[4*i:]))
	return string(p.data[start:end])
}

func (p *Packed) Release(a Allocator) {
	a.Free(p.index)
	a.Free(p.data)
}

// countingAlloc never fails; it just counts calls.
type countingAlloc struct{ calls, frees int }

func (c *countingAlloc) Alloc(n int) ([]byte, error) { c.calls++; return make([]byte, n), nil }
func (c *countingAlloc) Free(b []byte)               { c.frees++ }

func main() {
	var a countingAlloc
	p, _ := Pack(&a, []string{"ab", "", "cde"})
	fmt.Println(p.Item(0), p.Item(2), "allocs:", a.calls, "frees:", a.frees)
}
