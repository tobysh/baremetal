package main

import (
	"math/rand"
	"testing"
)

func TestBitmapModel(t *testing.T) {
	for _, n := range []uint32{9, 63, 128, 511} {
		d := NewDisk(int(n))
		FormatSuper(d, n, 4)
		s, _ := ReadSuper(d)
		before := d.Writes
		MarkMetadata(d, s)
		if d.Writes != before+1 {
			t.Fatal("mark writes")
		}
		used := make([]bool, n)
		for b := uint32(0); b < s.DataStart; b++ {
			used[b] = true
		}
		r := rand.New(rand.NewSource(int64(n)))
		for step := 0; step < 800; step++ {
			before = d.Writes
			if r.Intn(2) == 0 {
				want := 0
				for b := int(s.DataStart); b < int(n); b++ {
					if !used[b] {
						want = b
						break
					}
				}
				got, e := AllocBlock(d, s)
				if want == 0 {
					if e != ErrNoSpace || d.Writes != before {
						t.Fatal("full")
					}
				} else {
					if e != nil || got != want || d.Writes != before+1 {
						t.Fatal("allocation", got, want, e)
					}
					used[got] = true
				}
			} else {
				b := r.Intn(int(n)+4) - 2
				e := FreeBlock(d, s, b)
				switch {
				case b < int(s.DataStart) || b >= int(n):
					if e != ErrReserved {
						t.Fatal("reserved")
					}
				case !used[b]:
					if e != ErrNotAllocated {
						t.Fatal("double free")
					}
				default:
					if e != nil || d.Writes != before+1 {
						t.Fatal("free")
					}
					used[b] = false
				}
				if e != nil && d.Writes != before {
					t.Fatal("failed write")
				}
			}
			free := 0
			for b := 0; b < int(n); b++ {
				v, e := GetBit(d, s, b)
				if e != nil || v != used[b] {
					t.Fatal("bit", b)
				}
				if !used[b] {
					free++
				}
			}
			if CountFree(d, s) != free {
				t.Fatal("count")
			}
		}
		if _, e := GetBit(d, s, -1); e != ErrRange {
			t.Fatal("range")
		}
		if e := SetBit(d, s, int(n), true); e != ErrRange {
			t.Fatal("range")
		}
	}
}
