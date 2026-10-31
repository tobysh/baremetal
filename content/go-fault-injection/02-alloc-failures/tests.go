package main

import (
	"errors"
	"reflect"
	"testing"
)

type fakeAlloc struct {
	failAt int // 1-based call number to fail; 0 = never
	calls  int
	sizes  []int
	live   map[*byte]int
	bad    []string
}

func newFake(failAt int) *fakeAlloc { return &fakeAlloc{failAt: failAt, live: map[*byte]int{}} }

func (f *fakeAlloc) Alloc(n int) ([]byte, error) {
	f.calls++
	f.sizes = append(f.sizes, n)
	if f.calls == f.failAt {
		return nil, ErrNoMem
	}
	c := n
	if c < 1 {
		c = 1
	}
	b := make([]byte, n, c)
	f.live[&b[:1][0]] = n
	return b, nil
}

func (f *fakeAlloc) Free(b []byte) {
	if cap(b) < 1 {
		f.bad = append(f.bad, "freed a nil or foreign slice")
		return
	}
	k := &b[:1][0]
	if _, ok := f.live[k]; !ok {
		f.bad = append(f.bad, "double free or foreign free")
		return
	}
	delete(f.live, k)
}

func try(fn func()) (panicked bool) {
	defer func() {
		if recover() != nil {
			panicked = true
		}
	}()
	fn()
	return
}

var itemSets = [][]string{
	{"ab", "", "cde"},
	{"x"},
	{},
	{"", ""},
	{"alpha", "beta", "gamma", "delta", "epsilon"},
}

func TestPack_success_content_and_call_order(t *testing.T) {
	for _, items := range itemSets {
		f := newFake(0)
		var p *Packed
		var err error
		if try(func() { p, err = Pack(f, items) }) || err != nil {
			t.Fatalf("%q: Pack failed on a healthy allocator: err=%v", items, err)
		}
		total := 0
		for _, s := range items {
			total += len(s)
		}
		if want := []int{total, 4 * len(items), 4}; !reflect.DeepEqual(f.sizes, want) {
			t.Fatalf("%q: allocation sizes %v, want %v in that order", items, f.sizes, want)
		}
		if len(f.live) != 2 {
			t.Fatalf("%q: %d blocks live after Pack, want 2 (data and index); the scratch block must be freed", items, len(f.live))
		}
		for i, s := range items {
			if got := p.Item(i); got != s {
				t.Fatalf("%q: Item(%d) = %q", items, i, got)
			}
		}
		p.Release(f)
		if len(f.live) != 0 || len(f.bad) != 0 {
			t.Fatalf("%q: after Release live=%d problems=%v", items, len(f.live), f.bad)
		}
	}
}

func TestPack_failure_at_every_allocation(t *testing.T) {
	for _, items := range itemSets {
		for k := 1; k <= 3; k++ {
			f := newFake(k)
			var p *Packed
			var err error
			if try(func() { p, err = Pack(f, items) }) {
				t.Fatalf("%q, allocation %d failing: Pack panicked instead of returning the error", items, k)
			}
			if !errors.Is(err, ErrNoMem) || p != nil {
				t.Fatalf("%q, allocation %d failing: got (%v, %v), want (nil, ErrNoMem)", items, k, p, err)
			}
			if len(f.live) != 0 {
				t.Fatalf("%q, allocation %d failing: %d blocks leaked", items, k, len(f.live))
			}
			if len(f.bad) != 0 {
				t.Fatalf("%q, allocation %d failing: %v", items, k, f.bad)
			}
			if f.calls != k {
				t.Fatalf("%q, allocation %d failing: %d Alloc calls, want to stop at %d", items, k, f.calls, k)
			}
		}
	}
}
