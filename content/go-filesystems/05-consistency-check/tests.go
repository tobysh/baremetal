package main

import (
	"math/rand"
	"reflect"
	"sort"
	"testing"
)

func same(a, b []int) bool {
	if len(a) == 0 && len(b) == 0 {
		return true
	}
	return reflect.DeepEqual(a, b)
}

func expect(t *testing.T, name string, im Image, orphan, dangling, dup, invalid []int) {
	t.Helper()
	r := Check(im)
	if r.BadGeometry || !same(r.Orphan, orphan) || !same(r.Dangling, dangling) || !same(r.Duplicate, dup) || !same(r.Invalid, invalid) {
		t.Fatalf("%s: got %+v\nwant Orphan=%v Dangling=%v Duplicate=%v Invalid=%v", name, r, orphan, dangling, dup, invalid)
	}
}

func bitmap(n int, used ...int) []bool {
	u := make([]bool, n)
	for _, b := range used {
		u[b] = true
	}
	return u
}

func TestWorkedExample(t *testing.T) {
	im := Image{NBlocks: 10, DataStart: 3, Used: bitmap(10, 0, 1, 2, 3, 4, 6), Inodes: [][]int{{3, 4}, {4, 5, 12}}}
	expect(t, "lesson example", im, []int{6}, []int{5}, []int{4}, []int{12})
}

func TestCleanImage(t *testing.T) {
	im := Image{NBlocks: 12, DataStart: 4, Used: bitmap(12, 0, 1, 2, 3, 4, 5, 9), Inodes: [][]int{{4, 5}, nil, {9}, {}}}
	expect(t, "consistent image", im, nil, nil, nil, nil)
	empty := Image{NBlocks: 6, DataStart: 3, Used: bitmap(6)}
	expect(t, "no inodes, metadata bits clear", empty, nil, nil, nil, nil)
}

func TestMetadataBitsIgnored(t *testing.T) {
	// Bits 0..3 are metadata: set or clear, they are never findings.
	im := Image{NBlocks: 8, DataStart: 4, Used: []bool{false, true, false, true, true, false, false, false}, Inodes: [][]int{{4}}}
	expect(t, "metadata bits", im, nil, nil, nil, nil)
}

func TestEachClassOnItsOwn(t *testing.T) {
	expect(t, "orphan only", Image{NBlocks: 8, DataStart: 2, Used: bitmap(8, 5), Inodes: [][]int{{3}, nil}}, []int{5}, []int{3}, nil, nil)
	expect(t, "dangling only", Image{NBlocks: 8, DataStart: 2, Used: bitmap(8), Inodes: [][]int{{7}}}, nil, []int{7}, nil, nil)
	expect(t, "duplicate across inodes, bitmap correct", Image{NBlocks: 8, DataStart: 2, Used: bitmap(8, 4), Inodes: [][]int{{4}, {4}}}, nil, nil, []int{4}, nil)
	expect(t, "duplicate inside one inode", Image{NBlocks: 8, DataStart: 2, Used: bitmap(8, 6), Inodes: [][]int{{6, 6}}}, nil, nil, []int{6}, nil)
	expect(t, "three owners is still reported once", Image{NBlocks: 8, DataStart: 2, Used: bitmap(8, 3), Inodes: [][]int{{3}, {3}, {3}}}, nil, nil, []int{3}, nil)
	expect(t, "dangling and duplicate together", Image{NBlocks: 8, DataStart: 2, Used: bitmap(8), Inodes: [][]int{{2}, {2}}}, nil, []int{2}, []int{2}, nil)
}

func TestInvalidPointers(t *testing.T) {
	im := Image{NBlocks: 10, DataStart: 4, Used: bitmap(10, 4, 9), Inodes: [][]int{{4, 9, 10, -1}, {0, 3, 10, 10, 99, -1}}}
	expect(t, "invalid sorted with repeats", im, nil, nil, nil, []int{-1, -1, 0, 3, 10, 10, 10, 99})
	// Boundaries: DataStart is valid, DataStart-1 and NBlocks are not.
	im = Image{NBlocks: 6, DataStart: 3, Used: bitmap(6, 3, 5), Inodes: [][]int{{3, 5}, {2, 6}}}
	expect(t, "boundaries", im, nil, nil, nil, []int{2, 6})
}

func TestGeometry(t *testing.T) {
	bad := []Image{
		{NBlocks: -1},
		{NBlocks: 3, DataStart: 4, Used: make([]bool, 3)},
		{NBlocks: 2, Used: nil},
		{NBlocks: 5, DataStart: 1, Used: make([]bool, 6), Inodes: [][]int{{1, 2}}},
		{NBlocks: 5, DataStart: 1, Used: make([]bool, 4), Inodes: [][]int{{9}}},
		{NBlocks: 5, DataStart: -2, Used: make([]bool, 5), Inodes: [][]int{{1}}},
	}
	for i, im := range bad {
		if got := Check(im); !reflect.DeepEqual(got, Report{BadGeometry: true}) {
			t.Fatalf("bad geometry case %d: got %+v, want only BadGeometry", i, got)
		}
	}
	// No data region at all is valid; every pointer is then invalid.
	expect(t, "DataStart == NBlocks", Image{NBlocks: 3, DataStart: 3, Used: bitmap(3, 0, 1, 2), Inodes: [][]int{{1, 3}}}, nil, nil, nil, []int{1, 3})
	expect(t, "empty disk", Image{}, nil, nil, nil, nil)
}

func TestCheckDoesNotMutate(t *testing.T) {
	im := Image{NBlocks: 10, DataStart: 3, Used: bitmap(10, 3, 4, 9), Inodes: [][]int{{9, 4, 4, -2, 20}, {3}}}
	used := append([]bool(nil), im.Used...)
	inodes := [][]int{append([]int(nil), im.Inodes[0]...), append([]int(nil), im.Inodes[1]...)}
	first := Check(im)
	second := Check(im)
	if !reflect.DeepEqual(first, second) {
		t.Fatal("two checks of the same image disagree")
	}
	if !reflect.DeepEqual(im.Used, used) || !reflect.DeepEqual(im.Inodes, inodes) {
		t.Fatal("Check modified the image (it must report, not repair, and must not sort the caller's inode lists)")
	}
}

func TestCheckRandomAgainstModel(t *testing.T) {
	rng := rand.New(rand.NewSource(23))
	for trial := 0; trial < 400; trial++ {
		nb := 20 + rng.Intn(20)
		ds := rng.Intn(8)
		im := Image{NBlocks: nb, DataStart: ds, Used: make([]bool, nb), Inodes: make([][]int, rng.Intn(9))}
		for i := range im.Used {
			im.Used[i] = rng.Intn(2) == 0
		}
		for i := range im.Inodes {
			for j := 0; j < rng.Intn(10); j++ {
				im.Inodes[i] = append(im.Inodes[i], rng.Intn(nb+12)-6)
			}
		}
		got := Check(im)
		if got.BadGeometry {
			t.Fatalf("trial %d: valid geometry reported bad", trial)
		}
		var orphan, dangling, dup, invalid []int
		for b := ds; b < nb; b++ {
			count := 0
			for _, in := range im.Inodes {
				for _, p := range in {
					if p == b {
						count++
					}
				}
			}
			if count == 0 && im.Used[b] {
				orphan = append(orphan, b)
			}
			if count > 0 && !im.Used[b] {
				dangling = append(dangling, b)
			}
			if count > 1 {
				dup = append(dup, b)
			}
		}
		for _, in := range im.Inodes {
			for _, b := range in {
				if b < ds || b >= nb {
					invalid = append(invalid, b)
				}
			}
		}
		sort.Ints(invalid)
		if !same(got.Orphan, orphan) || !same(got.Dangling, dangling) || !same(got.Duplicate, dup) || !same(got.Invalid, invalid) {
			t.Fatalf("trial %d: image %+v\n got %+v\nwant Orphan=%v Dangling=%v Duplicate=%v Invalid=%v", trial, im, got, orphan, dangling, dup, invalid)
		}
	}
}
