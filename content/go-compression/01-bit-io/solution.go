package main

import (
	"errors"
	"fmt"
)

var ErrShort = errors.New("input ended early")

// BitWriter packs bits most-significant-first.
type BitWriter struct {
	buf   []byte
	nbits int
}

// WriteBits appends the low n bits of v (0 <= n <= 64), highest of those bits first.
func (w *BitWriter) WriteBits(v uint64, n int) {
	for i := n - 1; i >= 0; i-- {
		if w.nbits%8 == 0 {
			w.buf = append(w.buf, 0)
		}
		w.buf[len(w.buf)-1] |= byte(v>>uint(i)&1) << uint(7-w.nbits%8)
		w.nbits++
	}
}

// Len is the number of bits written.
func (w *BitWriter) Len() int { return w.nbits }

// Bytes returns a copy of the bytes so far; the last byte is padded with zero bits.
func (w *BitWriter) Bytes() []byte { return append([]byte(nil), w.buf...) }

type BitReader struct {
	data []byte
	pos  int
}

func NewBitReader(data []byte) *BitReader { return &BitReader{data: data} }

// ReadBits reads n bits (0 <= n <= 64). If fewer than n remain it returns ErrShort and consumes nothing.
func (r *BitReader) ReadBits(n int) (uint64, error) {
	if n > r.Remaining() {
		return 0, ErrShort
	}
	var v uint64
	for i := 0; i < n; i++ {
		v = v<<1 | uint64(r.data[r.pos/8]>>uint(7-r.pos%8)&1)
		r.pos++
	}
	return v, nil
}

func (r *BitReader) Pos() int       { return r.pos }
func (r *BitReader) Remaining() int { return len(r.data)*8 - r.pos }

func main() {
	var w BitWriter
	w.WriteBits(0b101, 3)
	w.WriteBits(0b1, 1)
	w.WriteBits(0xFF, 8)
	fmt.Printf("%d bits: % x\n", w.Len(), w.Bytes())
	r := NewBitReader(w.Bytes())
	a, _ := r.ReadBits(3)
	b, _ := r.ReadBits(1)
	fmt.Printf("read back %03b %b, %d bits left\n", a, b, r.Remaining())
}
