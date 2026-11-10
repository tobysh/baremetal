package main

import (
	"bytes"
	"math/rand"
	"testing"
)

func fresh(t *testing.T, n, inodes uint32) (*MemDisk, Super) {
	t.Helper()
	d := NewDisk(int(n))
	if e := FormatSuper(d, n, inodes); e != nil {
		t.Fatal(e)
	}
	s, e := ReadSuper(d)
	if e != nil {
		t.Fatal(e)
	}
	MarkMetadata(d, s)
	return d, s
}

func TestMarkMetadataRawBytes(t *testing.T) {
	d, s := fresh(t, 20, 4) // DataStart = 4
	if d.Data[1][0] != 0x0f || !bytes.Equal(d.Data[1][1:], make([]byte, 63)) {
		t.Fatalf("bitmap after MarkMetadata = %x, want 0f then zeros", d.Data[1])
	}
	for i := range d.Data[1] { // stale garbage must be wiped
		d.Data[1][i] = 0xff
	}
	w := d.Writes
	MarkMetadata(d, s)
	if d.Writes != w+1 {
		t.Fatalf("MarkMetadata did %d writes, want 1", d.Writes-w)
	}
	if d.Data[1][0] != 0x0f || !bytes.Equal(d.Data[1][1:], make([]byte, 63)) {
		t.Fatalf("MarkMetadata must clear every other bit, got %x", d.Data[1])
	}
}

func TestAllocLowestFirstAndRawBits(t *testing.T) {
	d, s := fresh(t, 24, 4) // data blocks 4..23
	for want := 4; want < 24; want++ {
		w := d.Writes
		got, e := AllocBlock(d, s)
		if e != nil || got != want {
			t.Fatalf("AllocBlock = %d, %v; want %d", got, e, want)
		}
		if d.Writes != w+1 {
			t.Fatalf("AllocBlock did %d writes, want 1", d.Writes-w)
		}
		if d.Data[1][got/8]&(1<<uint(got%8)) == 0 {
			t.Fatalf("raw bit %d (byte %d) not set after AllocBlock", got, got/8)
		}
	}
	w := d.Writes
	if _, e := AllocBlock(d, s); e != ErrNoSpace || d.Writes != w {
		t.Fatalf("full disk: want ErrNoSpace and no write, got %v with %d writes", e, d.Writes-w)
	}
	if CountFree(d, s) != 0 {
		t.Fatal("CountFree on a full disk must be 0")
	}
}

func TestFreeThenReuseLowest(t *testing.T) {
	d, s := fresh(t, 30, 6) // DataStart = 5
	for i := 0; i < 10; i++ {
		AllocBlock(d, s)
	}
	for _, b := range []int{9, 6, 12} {
		if e := FreeBlock(d, s, b); e != nil {
			t.Fatalf("FreeBlock(%d) = %v", b, e)
		}
	}
	for _, want := range []int{6, 9, 12, 15} {
		if got, _ := AllocBlock(d, s); got != want {
			t.Fatalf("after freeing 6, 9, 12 AllocBlock gave %d, want the lowest free block %d", got, want)
		}
	}
}

func TestFreeErrorsDoNotWrite(t *testing.T) {
	d, s := fresh(t, 20, 4)
	b, _ := AllocBlock(d, s)
	cases := []struct {
		name string
		b    int
		want error
	}{
		{"superblock", 0, ErrReserved},
		{"bitmap block", 1, ErrReserved},
		{"last metadata block", int(s.DataStart) - 1, ErrReserved},
		{"negative", -3, ErrReserved},
		{"at NBlocks", 20, ErrReserved},
		{"beyond NBlocks", 500, ErrReserved},
		{"never allocated", b + 3, ErrNotAllocated},
	}
	for _, c := range cases {
		w := d.Writes
		if e := FreeBlock(d, s, c.b); e != c.want {
			t.Fatalf("%s: FreeBlock(%d) = %v, want %v", c.name, c.b, e, c.want)
		}
		if d.Writes != w {
			t.Fatalf("%s: a failed FreeBlock wrote", c.name)
		}
	}
	if e := FreeBlock(d, s, b); e != nil {
		t.Fatal(e)
	}
	if e := FreeBlock(d, s, b); e != ErrNotAllocated {
		t.Fatalf("second free must be ErrNotAllocated, got %v", e)
	}
}

func TestGetSetBitRange(t *testing.T) {
	d, s := fresh(t, 40, 4)
	for _, b := range []int{-1, 40, 41, 511, 512, 1000} {
		w := d.Writes
		if _, e := GetBit(d, s, b); e != ErrRange {
			t.Fatalf("GetBit(%d) err = %v, want ErrRange", b, e)
		}
		if e := SetBit(d, s, b, true); e != ErrRange {
			t.Fatalf("SetBit(%d) err = %v, want ErrRange", b, e)
		}
		if d.Writes != w {
			t.Fatalf("out-of-range bit %d caused a write", b)
		}
	}
	w := d.Writes
	for _, b := range []int{0, 7, 8, 39} {
		if e := SetBit(d, s, b, true); e != nil {
			t.Fatal(e)
		}
		if v, _ := GetBit(d, s, b); !v {
			t.Fatalf("bit %d not set", b)
		}
		if e := SetBit(d, s, b, false); e != nil {
			t.Fatal(e)
		}
		if v, _ := GetBit(d, s, b); v {
			t.Fatalf("bit %d not cleared", b)
		}
	}
	if d.Writes != w+8 {
		t.Fatalf("SetBit must do one write per call (including no-op calls): got %d for 8 calls", d.Writes-w)
	}
}

func TestFilesystemSmallerThanDevice(t *testing.T) {
	d := NewDisk(100)
	FormatSuper(d, 20, 4)
	s, _ := ReadSuper(d)
	MarkMetadata(d, s)
	n := 0
	for {
		b, e := AllocBlock(d, s)
		if e != nil {
			break
		}
		if b >= 20 {
			t.Fatalf("AllocBlock returned %d, past NBlocks=20", b)
		}
		n++
	}
	if n != 16 || CountFree(d, s) != 0 {
		t.Fatalf("allocated %d blocks, want 16 (blocks 4..19)", n)
	}
}

func TestCountFreeIgnoresMetadataAndWrites(t *testing.T) {
	d, s := fresh(t, 50, 10) // DataStart = 7
	if got := CountFree(d, s); got != 43 {
		t.Fatalf("CountFree on a fresh disk = %d, want 43", got)
	}
	w := d.Writes
	for i := 0; i < 5; i++ {
		AllocBlock(d, s)
	}
	w = d.Writes
	if got := CountFree(d, s); got != 38 || d.Writes != w {
		t.Fatalf("CountFree = %d (writes %d), want 38 and no writes", got, d.Writes-w)
	}
}

func TestBitmapModel(t *testing.T) {
	for _, n := range []uint32{9, 63, 128, 511, 512} {
		d, s := fresh(t, n, 4)
		used := make([]bool, n)
		for b := uint32(0); b < s.DataStart; b++ {
			used[b] = true
		}
		r := rand.New(rand.NewSource(int64(n)))
		for step := 0; step < 800; step++ {
			before := d.Writes
			if r.Intn(2) == 0 {
				want := -1
				for b := int(s.DataStart); b < int(n); b++ {
					if !used[b] {
						want = b
						break
					}
				}
				got, e := AllocBlock(d, s)
				if want < 0 {
					if e != ErrNoSpace || d.Writes != before {
						t.Fatalf("n=%d: full disk gave %d, %v", n, got, e)
					}
				} else {
					if e != nil || got != want || d.Writes != before+1 {
						t.Fatalf("n=%d: AllocBlock = %d, %v; want %d", n, got, e, want)
					}
					used[got] = true
				}
			} else {
				b := r.Intn(int(n)+4) - 2
				e := FreeBlock(d, s, b)
				switch {
				case b < int(s.DataStart) || b >= int(n):
					if e != ErrReserved {
						t.Fatalf("n=%d: FreeBlock(%d) = %v, want ErrReserved", n, b, e)
					}
				case !used[b]:
					if e != ErrNotAllocated {
						t.Fatalf("n=%d: FreeBlock(%d) = %v, want ErrNotAllocated", n, b, e)
					}
				default:
					if e != nil || d.Writes != before+1 {
						t.Fatalf("n=%d: FreeBlock(%d) = %v with %d writes", n, b, e, d.Writes-before)
					}
					used[b] = false
				}
				if e != nil && d.Writes != before {
					t.Fatalf("n=%d: failed FreeBlock(%d) wrote", n, b)
				}
			}
			free := 0
			for b := 0; b < int(n); b++ {
				v, e := GetBit(d, s, b)
				if e != nil || v != used[b] {
					t.Fatalf("n=%d: bit %d = %v, model says %v", n, b, v, used[b])
				}
				if !used[b] && b >= int(s.DataStart) {
					free++
				}
			}
			if CountFree(d, s) != free {
				t.Fatalf("n=%d: CountFree = %d, model says %d", n, CountFree(d, s), free)
			}
		}
	}
}
