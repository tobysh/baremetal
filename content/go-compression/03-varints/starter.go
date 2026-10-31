package main

import (
	"errors"
	"fmt"
)

var (
	ErrShort        = errors.New("input ended early")
	ErrOverflow     = errors.New("varint overflows 64 bits")
	ErrNonCanonical = errors.New("varint is not the shortest encoding")
)

func AppendUvarint(dst []byte, v uint64) []byte { return dst }

func Uvarint(b []byte) (uint64, int, error) { return 0, 0, ErrShort }

// AppendVarint / Varint use zigzag: 0,-1,1,-2,2... map to 0,1,2,3,4...
func AppendVarint(dst []byte, v int64) []byte { return dst }

func Varint(b []byte) (int64, int, error) { return 0, 0, ErrShort }

func main() {
	b := AppendUvarint(nil, 300)
	fmt.Printf("300 -> % x\n", b)
	b = AppendVarint(nil, -3)
	v, n, err := Varint(b)
	fmt.Printf("-3 -> % x -> %d (%d byte) %v\n", b, v, n, err)
}
