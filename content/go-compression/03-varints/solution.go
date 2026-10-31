package main

import (
	"errors"
	"fmt"
)

var ErrShort = errors.New("input ended early")

var (
	ErrOverflow     = errors.New("varint overflows 64 bits")
	ErrNonCanonical = errors.New("varint is not the shortest encoding")
)

func AppendUvarint(dst []byte, v uint64) []byte {
	for v >= 0x80 {
		dst = append(dst, byte(v)|0x80)
		v >>= 7
	}
	return append(dst, byte(v))
}

func Uvarint(b []byte) (uint64, int, error) {
	var v uint64
	for i := 0; i < len(b); i++ {
		c := b[i]
		if i == 9 && c > 1 {
			return 0, 0, ErrOverflow
		}
		v |= uint64(c&0x7f) << (7 * uint(i))
		if c < 0x80 {
			if i > 0 && c == 0 {
				return 0, 0, ErrNonCanonical
			}
			return v, i + 1, nil
		}
	}
	return 0, 0, ErrShort
}

func AppendVarint(dst []byte, v int64) []byte {
	return AppendUvarint(dst, uint64(v<<1)^uint64(v>>63))
}

func Varint(b []byte) (int64, int, error) {
	u, n, err := Uvarint(b)
	if err != nil {
		return 0, 0, err
	}
	return int64(u>>1) ^ -int64(u&1), n, nil
}

func main() {
	b := AppendUvarint(nil, 300)
	fmt.Printf("300 -> % x\n", b)
	b = AppendVarint(nil, -3)
	v, n, err := Varint(b)
	fmt.Printf("-3 -> % x -> %d (%d byte) %v\n", b, v, n, err)
}
