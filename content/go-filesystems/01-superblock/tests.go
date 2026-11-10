package main

import (
	"bytes"
	"testing"
)

// filled returns a disk whose every byte is 0xa5, so untouched blocks are easy to spot.
func filled(n int) *MemDisk {
	d := NewDisk(n)
	for _, b := range d.Data {
		for j := range b {
			b[j] = 0xa5
		}
	}
	return d
}

type lastWrite struct {
	*MemDisk
	order []int
}

func (d *lastWrite) WriteBlock(b int, v []byte) {
	d.order = append(d.order, b)
	d.MemDisk.WriteBlock(b, v)
}

func TestFormatRoundTrip(t *testing.T) {
	for n := uint32(8); n <= 512; n += 7 {
		for _, ni := range []uint32{2, 5, 8, 6} {
			d := NewDisk(int(n))
			if e := FormatSuper(d, n, ni); e != nil {
				t.Fatalf("FormatSuper(%d,%d) = %v, want nil", n, ni, e)
			}
			s, e := ReadSuper(d)
			ds := 2 + ni/2 + ni%2
			want := Super{n, ni, 1, 2, ds}
			if e != nil || s != want {
				t.Fatalf("ReadSuper after FormatSuper(%d,%d) = %+v, %v; want %+v", n, ni, s, e, want)
			}
		}
	}
}

func TestFormatRawBytes(t *testing.T) {
	d := filled(40)
	if e := FormatSuper(d, 33, 5); e != nil {
		t.Fatal(e)
	}
	want := []uint32{0x424d4653, 1, 33, 5, 1, 2, 5}
	for i, x := range want {
		if got := u32(d.Data[0][i*4:]); got != x {
			t.Fatalf("superblock word %d = %#x, want %#x (big-endian)", i, got, x)
		}
	}
	if !bytes.Equal(d.Data[0][28:], make([]byte, 36)) {
		t.Fatalf("the tail of block 0 must be zero, got %x", d.Data[0][28:])
	}
	if d.Data[0][0] != 'B' || d.Data[0][1] != 'M' || d.Data[0][2] != 'F' || d.Data[0][3] != 'S' {
		t.Fatalf("the first four bytes must spell BMFS, got %q", d.Data[0][:4])
	}
}

func TestFormatZeroesMetadataOnly(t *testing.T) {
	d := filled(48)
	if e := FormatSuper(d, 40, 7); e != nil { // DataStart = 2 + 4 = 6
		t.Fatal(e)
	}
	for b := 1; b < 6; b++ {
		if !bytes.Equal(d.Data[b], make([]byte, 64)) {
			t.Fatalf("metadata block %d must be zeroed", b)
		}
	}
	for b := 6; b < 48; b++ {
		if !bytes.Equal(d.Data[b], bytes.Repeat([]byte{0xa5}, 64)) {
			t.Fatalf("block %d is outside the metadata and must not be written (this includes blocks at or past nblocks)", b)
		}
	}
}

func TestFormatSuperblockWrittenLast(t *testing.T) {
	d := &lastWrite{MemDisk: NewDisk(20)}
	if e := FormatSuper(d, 20, 8); e != nil {
		t.Fatal(e)
	}
	if len(d.order) == 0 || d.order[len(d.order)-1] != 0 {
		t.Fatalf("block 0 must be written last, write order was %v", d.order)
	}
}

func TestFormatBoundaries(t *testing.T) {
	d := NewDisk(8)
	if e := FormatSuper(d, 7, 8); e != nil { // DataStart = 6, exactly one data block (6)
		t.Fatalf("one data block must be accepted, got %v", e)
	}
	if s, e := ReadSuper(d); e != nil || s.DataStart != 6 || s.NBlocks != 7 {
		t.Fatalf("got %+v, %v", s, e)
	}
	d = NewDisk(512)
	if e := FormatSuper(d, 512, 2); e != nil {
		t.Fatalf("512 blocks is the maximum and must be accepted, got %v", e)
	}
}

func TestFormatRejectsWithoutWriting(t *testing.T) {
	cases := []struct {
		name         string
		dev          int
		blocks, inos uint32
		want         error
	}{
		{"more than 512 blocks", 600, 513, 8, ErrBadSuper},
		{"one inode", 32, 32, 1, ErrBadSuper},
		{"zero inodes", 32, 32, 0, ErrBadSuper},
		{"larger than device", 32, 33, 4, ErrBadSuper},
		{"no room for data", 32, 8, 20, ErrNoSpace},
		{"data start equals nblocks", 32, 7, 10, ErrNoSpace}, // 2 + 5 = 7
		{"zero blocks", 32, 0, 2, ErrNoSpace},
		{"huge inode count", 32, 32, 0xffffffff, ErrNoSpace},
		{"both problems: bad wins", 32, 600, 4000, ErrBadSuper},
	}
	for _, c := range cases {
		d := filled(c.dev)
		e := FormatSuper(d, c.blocks, c.inos)
		if e == nil {
			t.Fatalf("%s: FormatSuper(%d,%d) succeeded, want %v", c.name, c.blocks, c.inos, c.want)
		}
		if e != c.want {
			t.Fatalf("%s: got error %v, want %v", c.name, e, c.want)
		}
		if d.Writes != 0 {
			t.Fatalf("%s: a rejected FormatSuper wrote %d blocks, want 0", c.name, d.Writes)
		}
	}
}

func TestReadSuperCorruptEachField(t *testing.T) {
	for _, bad := range []uint32{0xffffffff, 0, 3} {
		d := NewDisk(64)
		FormatSuper(d, 40, 6)
		original := append([]byte(nil), d.Data[0]...)
		for _, off := range []int{0, 4, 8, 12, 16, 20, 24} {
			copy(d.Data[0], original)
			putU32(d.Data[0][off:], bad)
			s, e := ReadSuper(d)
			if e != ErrBadSuper {
				t.Fatalf("word at byte %d set to %#x was accepted (%+v, %v); every field must be checked", off, bad, s, e)
			}
		}
	}
}

func TestReadSuperDeviceMismatch(t *testing.T) {
	d := NewDisk(64)
	FormatSuper(d, 64, 4)
	small := NewDisk(32)
	copy(small.Data[0], d.Data[0])
	if _, e := ReadSuper(small); e != ErrBadSuper {
		t.Fatalf("NBlocks=64 on a 32-block device must be ErrBadSuper, got %v", e)
	}
	big := NewDisk(100)
	copy(big.Data[0], d.Data[0])
	if s, e := ReadSuper(big); e != nil || s.NBlocks != 64 {
		t.Fatalf("a larger device is fine: got %+v, %v", s, e)
	}
}

func TestReadSuperBlankAndReadOnly(t *testing.T) {
	d := NewDisk(16)
	if _, e := ReadSuper(d); e != ErrBadSuper {
		t.Fatalf("an all-zero disk must be ErrBadSuper, got %v", e)
	}
	FormatSuper(d, 16, 4)
	w := d.Writes
	ReadSuper(d)
	if d.Writes != w {
		t.Fatal("ReadSuper must not write")
	}
}
