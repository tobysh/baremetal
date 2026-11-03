package main

import (
	"bytes"
	"testing"
)

func TestFiles(t *testing.T) {
	d := NewDisk(40)
	FormatSuper(d, 40, 8)
	s, _ := ReadSuper(d)
	MarkMetadata(d, s)
	a, _ := AllocInode(d, s, 1)
	b, _ := AllocInode(d, s, 1)
	neighbor, _ := ReadInode(d, s, b)
	empty := CountFree(d, s)
	for _, size := range []int{1, 64, 65, 384, 0, 137, 2} {
		v := bytes.Repeat([]byte{byte(size)}, size)
		if e := WriteFile(d, s, a, v); e != nil {
			t.Fatal(e)
		}
		got, e := ReadFile(d, s, a)
		if e != nil || !bytes.Equal(got, v) {
			t.Fatal("contents", size, e)
		}
		if CountFree(d, s) != empty-(size+63)/64 {
			t.Fatal("leak")
		}
		in, _ := ReadInode(d, s, a)
		block, off := inodePos(s, a)
		raw := d.Data[block][off:]
		if raw[0] != 1 || raw[1] != 1 || u32(raw[4:]) != uint32(size) {
			t.Fatal("layout")
		}
		for j, x := range in.Direct {
			if u32(raw[8+j*4:]) != x {
				t.Fatal("direct")
			}
			if j >= (size+63)/64 && x != 0 {
				t.Fatal("unused")
			}
		}
		other, _ := ReadInode(d, s, b)
		if other != neighbor {
			t.Fatal("neighbor")
		}
	}
	before, _ := ReadFile(d, s, a)
	free := CountFree(d, s)
	if WriteFile(d, s, a, make([]byte, 385)) != ErrTooBig {
		t.Fatal("too big")
	}
	for CountFree(d, s) > 0 {
		AllocBlock(d, s)
	}
	if WriteFile(d, s, a, make([]byte, 64)) != ErrNoSpace {
		t.Fatal("space")
	}
	after, _ := ReadFile(d, s, a)
	if !bytes.Equal(before, after) || CountFree(d, s) != 0 {
		t.Fatal("failed replacement")
	}
	_ = free
	if FreeInode(d, s, a) != nil || FreeInode(d, s, a) != ErrNotFound {
		t.Fatal("free")
	}
	if _, e := ReadInode(d, s, -1); e != ErrRange {
		t.Fatal("range")
	}
}

type recordedDisk struct {
	*MemDisk
	order []int
}

func (d *recordedDisk) WriteBlock(b int, v []byte) {
	d.order = append(d.order, b)
	d.MemDisk.WriteBlock(b, v)
}
func TestReplacementOrdering(t *testing.T) {
	d := &recordedDisk{MemDisk: NewDisk(32)}
	FormatSuper(d, 32, 8)
	s, _ := ReadSuper(d)
	MarkMetadata(d, s)
	n, _ := AllocInode(d, s, 1)
	WriteFile(d, s, n, []byte("old"))
	old, _ := ReadInode(d, s, n)
	d.order = nil
	if e := WriteFile(d, s, n, bytes.Repeat([]byte{7}, 65)); e != nil {
		t.Fatal(e)
	}
	in, _ := ReadInode(d, s, n)
	block, _ := inodePos(s, n)
	want := []int{int(s.BitmapStart), int(s.BitmapStart), int(in.Direct[0]), int(in.Direct[1]), block, int(s.BitmapStart)}
	if len(d.order) != len(want) {
		t.Fatal("write count", d.order)
	}
	for i, b := range want {
		if d.order[i] != b {
			t.Fatal("write order", d.order, want)
		}
	}
	used, _ := GetBit(d, s, int(old.Direct[0]))
	if used {
		t.Fatal("old block retained")
	}
	last := d.ReadBlock(int(in.Direct[1]))
	if last[0] != 7 || !bytes.Equal(last[1:], make([]byte, 63)) {
		t.Fatal("padding")
	}
}
