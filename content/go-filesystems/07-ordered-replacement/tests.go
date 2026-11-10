package main

import (
	"fmt"
	"testing"
)

// disk is the model state after some prefix of writes.
type disk struct {
	used map[int]bool
	data map[int]int
	ptr  int
}

func replay(old int, ws []Write) disk {
	d := disk{used: map[int]bool{old: true}, data: map[int]int{old: 7}, ptr: old}
	for _, w := range ws {
		switch w.Kind {
		case "alloc":
			d.used[w.Block] = true
		case "data":
			d.data[w.Block] = w.Value
		case "inode":
			d.ptr = w.Block
		case "free":
			delete(d.used, w.Block)
		}
	}
	return d
}

func eachCase(f func(old, fresh, value int)) {
	for old := 0; old < 8; old++ {
		for fresh := 0; fresh < 8; fresh++ {
			if old == fresh {
				continue
			}
			for _, value := range []int{-13, 0, 91, 1 << 20} {
				f(old, fresh, value)
			}
		}
	}
}

func TestFourWritesInContract(t *testing.T) {
	kinds := []string{"alloc", "data", "inode", "free"}
	eachCase(func(old, fresh, value int) {
		ws := Replacement(old, fresh, value)
		if len(ws) != 4 {
			t.Fatalf("Replacement(%d,%d,%d) returned %d writes, want 4: %v", old, fresh, value, len(ws), ws)
		}
		for i, w := range ws {
			b, v := fresh, 0
			if i == 3 {
				b = old
			}
			if i == 1 {
				v = value
			}
			if w.Kind != kinds[i] || w.Block != b || w.Value != v {
				t.Fatalf("Replacement(%d,%d,%d) write %d = %+v, want {%s %d %d}", old, fresh, value, i, w, kinds[i], b, v)
			}
		}
	})
}

func TestEveryCutKeepsPointerValid(t *testing.T) {
	eachCase(func(old, fresh, value int) {
		ws := Replacement(old, fresh, value)
		if len(ws) != 4 {
			t.Fatalf("Replacement(%d,%d,%d) returned %d writes, want 4", old, fresh, value, len(ws))
		}
		for cut := 0; cut <= len(ws); cut++ {
			d := replay(old, ws[:cut])
			if !d.used[d.ptr] {
				t.Fatalf("(%d,%d,%d) cut %d: inode points at block %d, which is not allocated (dangling pointer)", old, fresh, value, cut, d.ptr)
			}
			v, ok := d.data[d.ptr]
			if !ok {
				t.Fatalf("(%d,%d,%d) cut %d: inode points at block %d, which was never written (uninitialized data)", old, fresh, value, cut, d.ptr)
			}
			if d.ptr == fresh && v != value {
				t.Fatalf("(%d,%d,%d) cut %d: pointer names the new block but it holds %d, want %d", old, fresh, value, cut, v, value)
			}
		}
	})
}

func TestFileIsEntirelyOldOrNew(t *testing.T) {
	eachCase(func(old, fresh, value int) {
		ws := Replacement(old, fresh, value)
		if len(ws) != 4 {
			t.Fatalf("Replacement(%d,%d,%d) returned %d writes, want 4", old, fresh, value, len(ws))
		}
		for cut := 0; cut <= 4; cut++ {
			d := replay(old, ws[:cut])
			want := old
			if cut >= 3 {
				want = fresh
			}
			if d.ptr != want {
				t.Fatalf("(%d,%d,%d) cut %d: pointer = %d, want %d (the inode write must be the third write and the switch)", old, fresh, value, cut, d.ptr, want)
			}
			if cut < 3 && d.data[old] != 7 {
				t.Fatalf("(%d,%d,%d) cut %d: the old contents were touched before the switch", old, fresh, value, cut)
			}
		}
	})
}

func TestFinalStateAndLeaks(t *testing.T) {
	eachCase(func(old, fresh, value int) {
		ws := Replacement(old, fresh, value)
		if len(ws) != 4 {
			t.Fatalf("Replacement(%d,%d,%d) returned %d writes, want 4", old, fresh, value, len(ws))
		}
		d := replay(old, ws)
		if len(d.used) != 1 || !d.used[fresh] || d.ptr != fresh || d.data[fresh] != value {
			t.Fatalf("(%d,%d,%d) final state %+v: want only block %d allocated, pointed at, holding %d", old, fresh, value, d, fresh, value)
		}
		for cut := 0; cut <= 4; cut++ {
			if n := len(replay(old, ws[:cut]).used); n < 1 || n > 2 {
				t.Fatalf("(%d,%d,%d) cut %d: %d blocks allocated, a crash may leak at most one", old, fresh, value, cut, n)
			}
		}
	})
}

func TestFreeComesLastAndCallsAreIndependent(t *testing.T) {
	a := Replacement(1, 2, 5)
	if len(a) != 4 {
		t.Fatalf("want 4 writes, got %v", a)
	}
	a[0].Block = 99
	a[3].Kind = "bogus"
	b := Replacement(1, 2, 5)
	if fmt.Sprint(b) != "[{alloc 2 0} {data 2 5} {inode 2 0} {free 1 0}]" {
		t.Fatalf("Replacement must return a fresh slice each call, second call gave %v", b)
	}
}
