package main

import (
	"bytes"
	"math/rand"
	"testing"
)

func newFS(t *testing.T, blocks, inodes uint32) (*MemDisk, Super) {
	t.Helper()
	d := NewDisk(int(blocks))
	if e := FormatSuper(d, blocks, inodes); e != nil {
		t.Fatal(e)
	}
	s, _ := ReadSuper(d)
	MarkMetadata(d, s)
	return d, s
}

func payload(seed int64, n int) []byte {
	b := make([]byte, n)
	rand.New(rand.NewSource(seed)).Read(b)
	return b
}

func TestInodeRawLayout(t *testing.T) {
	d := NewDisk(30)
	FormatSuper(d, 30, 8)
	s, _ := ReadSuper(d)
	in := Inode{Type: 2, Links: 3, Size: 130, Direct: [6]uint32{9, 10, 11, 0, 0, 0}}
	for _, n := range []int{0, 1, 2, 7} {
		w := d.Writes
		if e := WriteInode(d, s, n, in); e != nil {
			t.Fatal(e)
		}
		if d.Writes != w+1 {
			t.Fatalf("WriteInode did %d writes, want 1", d.Writes-w)
		}
		block, off := int(s.InodeStart)+n/2, (n%2)*32
		raw := d.Data[block][off : off+32]
		if raw[0] != 2 || raw[1] != 3 || raw[2] != 0 || raw[3] != 0 || u32(raw[4:]) != 130 {
			t.Fatalf("inode %d header bytes wrong: %x", n, raw[:8])
		}
		for j, x := range in.Direct {
			if u32(raw[8+j*4:]) != x {
				t.Fatalf("inode %d Direct[%d] = %d on disk, want %d (big-endian)", n, j, u32(raw[8+j*4:]), x)
			}
		}
		got, e := ReadInode(d, s, n)
		if e != nil || got != in {
			t.Fatalf("ReadInode(%d) = %+v, %v; want %+v", n, got, e, in)
		}
	}
}

func TestInodeNeighbourUntouched(t *testing.T) {
	d, s := newFS(t, 30, 8)
	a := Inode{Type: 1, Links: 1, Size: 5, Direct: [6]uint32{20, 0, 0, 0, 0, 0}}
	b := Inode{Type: 2, Links: 9, Size: 64, Direct: [6]uint32{21, 0, 0, 0, 0, 0}}
	WriteInode(d, s, 2, a)
	WriteInode(d, s, 3, b)
	WriteInode(d, s, 2, Inode{Type: 1, Links: 2, Size: 1})
	if got, _ := ReadInode(d, s, 3); got != b {
		t.Fatalf("writing inode 2 changed inode 3: %+v", got)
	}
	WriteInode(d, s, 3, Inode{})
	if got, _ := ReadInode(d, s, 2); got.Links != 2 || got.Size != 1 {
		t.Fatalf("writing inode 3 changed inode 2: %+v", got)
	}
	// The 2 zero bytes and unused Direct entries must be overwritten, not merged.
	WriteInode(d, s, 2, Inode{Type: 1})
	block, off := int(s.InodeStart)+1, 0
	if !bytes.Equal(d.Data[block][off+1:off+32], make([]byte, 31)) {
		t.Fatalf("WriteInode must replace all 32 bytes, left %x", d.Data[block][off:off+32])
	}
}

func TestInodeRange(t *testing.T) {
	d, s := newFS(t, 30, 8)
	w := d.Writes
	for _, n := range []int{-1, 8, 9, 100} {
		if _, e := ReadInode(d, s, n); e != ErrRange {
			t.Fatalf("ReadInode(%d) = %v, want ErrRange", n, e)
		}
		if e := WriteInode(d, s, n, Inode{Type: 1}); e != ErrRange {
			t.Fatalf("WriteInode(%d) = %v, want ErrRange", n, e)
		}
	}
	if d.Writes != w {
		t.Fatal("out-of-range inode numbers must not write")
	}
}

func TestFileSizes(t *testing.T) {
	d, s := newFS(t, 40, 8)
	a, _ := AllocInode(d, s, 1)
	b, _ := AllocInode(d, s, 1)
	WriteFile(d, s, b, []byte("neighbour"))
	neighbour, _ := ReadInode(d, s, b)
	empty := CountFree(d, s) + len(neighbour.blocks())
	for i, size := range []int{1, 64, 65, 384, 0, 137, 2, 128, 383, 63} {
		v := payload(int64(i), size)
		if e := WriteFile(d, s, a, v); e != nil {
			t.Fatalf("size %d: %v", size, e)
		}
		got, e := ReadFile(d, s, a)
		if e != nil || !bytes.Equal(got, v) {
			t.Fatalf("size %d: contents differ (err %v)", size, e)
		}
		if want := empty - len(neighbour.blocks()) - (size+63)/64; CountFree(d, s) != want {
			t.Fatalf("size %d: %d free blocks, want %d (leak or double free)", size, CountFree(d, s), want)
		}
		in, _ := ReadInode(d, s, a)
		block, off := inodePos(s, a)
		raw := d.Data[block][off:]
		if raw[0] != 1 || raw[1] != 1 || u32(raw[4:]) != uint32(size) {
			t.Fatalf("size %d: inode header wrong, Type/Links must be kept and Size set: %x", size, raw[:8])
		}
		for j, x := range in.Direct {
			if u32(raw[8+j*4:]) != x {
				t.Fatalf("size %d: Direct[%d] mismatch on disk", size, j)
			}
			if j >= (size+63)/64 && x != 0 {
				t.Fatalf("size %d: unused Direct[%d] = %d, want 0", size, j, x)
			}
		}
		if other, _ := ReadInode(d, s, b); other != neighbour {
			t.Fatal("WriteFile changed another file's inode")
		}
		if nb, _ := ReadFile(d, s, b); string(nb) != "neighbour" {
			t.Fatalf("WriteFile damaged another file: %q", nb)
		}
	}
}

func TestRewriteFreesOldBlocks(t *testing.T) {
	d, s := newFS(t, 30, 4)
	n, _ := AllocInode(d, s, 1)
	base := CountFree(d, s)
	for _, size := range []int{300, 10, 200, 0, 64} {
		WriteFile(d, s, n, payload(int64(size), size))
	}
	if CountFree(d, s) != base-1 {
		t.Fatalf("after rewrites ending at 64 bytes, %d blocks free, want %d", CountFree(d, s), base-1)
	}
	if e := FreeInode(d, s, n); e != nil || CountFree(d, s) != base {
		t.Fatalf("FreeInode: %v, free %d want %d", e, CountFree(d, s), base)
	}
}

func TestFailedWritesChangeNothing(t *testing.T) {
	d, s := newFS(t, 20, 4) // data blocks 4..19 = 16 blocks
	n, _ := AllocInode(d, s, 1)
	orig := payload(7, 200) // 4 blocks
	WriteFile(d, s, n, orig)
	snapshot := func() ([]byte, Inode, int) {
		in, _ := ReadInode(d, s, n)
		c, _ := ReadFile(d, s, n)
		return c, in, CountFree(d, s)
	}
	c0, in0, f0 := snapshot()
	if e := WriteFile(d, s, n, make([]byte, 385)); e != ErrTooBig {
		t.Fatalf("385 bytes: %v, want ErrTooBig", e)
	}
	// Leave only 3 free blocks: a 4-block rewrite needs old+new, so it cannot fit.
	extra := f0 - 3
	for i := 0; i < extra; i++ {
		AllocBlock(d, s)
	}
	c0, in0, f0 = snapshot()
	if e := WriteFile(d, s, n, payload(8, 256)); e != ErrNoSpace {
		t.Fatalf("rewrite needing 4 blocks with 3 free: %v, want ErrNoSpace", e)
	}
	c1, in1, f1 := snapshot()
	if !bytes.Equal(c0, c1) || in0 != in1 || f0 != f1 {
		t.Fatalf("a failed WriteFile changed the file or leaked blocks (free %d -> %d)", f0, f1)
	}
	// A smaller rewrite that fits must still work.
	if e := WriteFile(d, s, n, payload(9, 130)); e != nil {
		t.Fatalf("3-block rewrite with 3 free blocks must succeed: %v", e)
	}
	if got, _ := ReadFile(d, s, n); !bytes.Equal(got, payload(9, 130)) {
		t.Fatal("contents after the successful rewrite differ")
	}
}

func TestFileOpsOnFreeInode(t *testing.T) {
	d, s := newFS(t, 20, 4)
	w := d.Writes
	if e := WriteFile(d, s, 2, []byte("x")); e != ErrNotFound {
		t.Fatalf("WriteFile on a free inode = %v, want ErrNotFound", e)
	}
	if e := FreeInode(d, s, 2); e != ErrNotFound {
		t.Fatalf("FreeInode on a free inode = %v, want ErrNotFound", e)
	}
	if d.Writes != w {
		t.Fatal("failed calls must not write")
	}
	if e := WriteFile(d, s, 99, nil); e != ErrRange {
		t.Fatalf("WriteFile(99) = %v, want ErrRange", e)
	}
}

func TestFreeInodeZeroesInode(t *testing.T) {
	d, s := newFS(t, 30, 8)
	a, _ := AllocInode(d, s, 1)
	b, _ := AllocInode(d, s, 1)
	WriteFile(d, s, a, payload(1, 150))
	WriteFile(d, s, b, payload(2, 70))
	before := CountFree(d, s)
	if e := FreeInode(d, s, a); e != nil {
		t.Fatal(e)
	}
	if CountFree(d, s) != before+3 {
		t.Fatalf("FreeInode must free the 3 data blocks: free %d -> %d", before, CountFree(d, s))
	}
	block, off := inodePos(s, a)
	if !bytes.Equal(d.Data[block][off:off+32], make([]byte, 32)) {
		t.Fatalf("FreeInode must zero the inode, found %x", d.Data[block][off:off+32])
	}
	if got, _ := ReadFile(d, s, b); !bytes.Equal(got, payload(2, 70)) {
		t.Fatal("FreeInode damaged another file")
	}
	again, _ := AllocInode(d, s, 1)
	if again != a {
		t.Fatalf("freed inode %d should be reused first, AllocInode gave %d", a, again)
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
	bm := int(s.BitmapStart)
	want := []int{bm, bm, int(in.Direct[0]), int(in.Direct[1]), block, bm}
	if len(d.order) != len(want) {
		t.Fatalf("write sequence %v, want %v (alloc, alloc, data, data, inode, free)", d.order, want)
	}
	for i, b := range want {
		if d.order[i] != b {
			t.Fatalf("write sequence %v, want %v", d.order, want)
		}
	}
	if used, _ := GetBit(d, s, int(old.Direct[0])); used {
		t.Fatal("old block retained")
	}
	last := d.ReadBlock(int(in.Direct[1]))
	if last[0] != 7 || !bytes.Equal(last[1:], make([]byte, 63)) {
		t.Fatal("the unused tail of the last data block must be zero padding")
	}
}
