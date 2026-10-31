package main

import (
	"errors"
	"fmt"
)

var ErrShort = errors.New("input ended early")

// BitWriter packs bits most-significant-first: the first bit written is the
// 0x80 bit of the first byte. Add whatever fields you need.
type BitWriter struct{}

// WriteBits appends the low n bits of v (0 <= n <= 64), highest of those bits first.
func (w *BitWriter) WriteBits(v uint64, n int) {}

// Len is the number of bits written.
func (w *BitWriter) Len() int { return 0 }

// Bytes returns a copy of the bytes so far; the last byte is padded with zero bits.
func (w *BitWriter) Bytes() []byte { return nil }

type BitReader struct{}

func NewBitReader(data []byte) *BitReader { return &BitReader{} }

// ReadBits reads n bits (0 <= n <= 64). If fewer than n remain it returns
// ErrShort and consumes nothing.
func (r *BitReader) ReadBits(n int) (uint64, error) { return 0, ErrShort }

func (r *BitReader) Pos() int       { return 0 } // bits consumed so far
func (r *BitReader) Remaining() int { return 0 } // bits left

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
