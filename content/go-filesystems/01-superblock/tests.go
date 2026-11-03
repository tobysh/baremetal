package main

import (
	"bytes"
	"testing"
)

func TestGeometry(t *testing.T) {
	for n := uint32(8); n <= 512; n += 7 {
		d := NewDisk(int(n) + 3)
		for _, b := range d.Data {
			for j := range b {
				b[j] = 0xa5
			}
		}
		if e := FormatSuper(d, n, 5); e != nil {
			t.Fatal(e)
		}
		s, e := ReadSuper(d)
		if e != nil || s != (Super{n, 5, 1, 2, 5}) {
			t.Fatal(s, e)
		}
		want := []uint32{0x424d4653, 1, n, 5, 1, 2, 5}
		for i, x := range want {
			if u32(d.Data[0][i*4:]) != x {
				t.Fatal("raw word", i)
			}
		}
		for b := 1; b < 5; b++ {
			if !bytes.Equal(d.Data[b], make([]byte, 64)) {
				t.Fatal("metadata")
			}
		}
		if d.Data[5][0] != 0xa5 {
			t.Fatal("data overwritten")
		}
		original := append([]byte(nil), d.Data[0]...)
		for _, off := range []int{0, 4, 8, 12, 16, 20, 24} {
			copy(d.Data[0], original)
			putU32(d.Data[0][off:], 0xffffffff)
			if _, e := ReadSuper(d); e != ErrBadSuper {
				t.Fatal("corrupt accepted", off)
			}
		}
	}
}
func TestInvalid(t *testing.T) {
	for _, v := range [][2]uint32{{513, 8}, {32, 1}, {8, 20}, {0, 2}, {32, 0xffffffff}} {
		d := NewDisk(32)
		if FormatSuper(d, v[0], v[1]) == nil || d.Writes != 0 {
			t.Fatal(v, d.Writes)
		}
	}
}
