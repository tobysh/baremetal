package main

import (
	"math/rand"
	"reflect"
	"sort"
	"testing"
)

func TestCheck(t *testing.T) {
	rng := rand.New(rand.NewSource(23))
	for trial := 0; trial < 300; trial++ {
		im := Image{NBlocks: 30, DataStart: 5, Used: make([]bool, 30), Inodes: make([][]int, 8)}
		for i := range im.Used {
			im.Used[i] = rng.Intn(2) == 0
		}
		for i := range im.Inodes {
			for j := 0; j < rng.Intn(10); j++ {
				im.Inodes[i] = append(im.Inodes[i], rng.Intn(40)-5)
			}
		}
		got := Check(im)
		for b := 5; b < 30; b++ {
			count := 0
			for _, in := range im.Inodes {
				for _, p := range in {
					if p == b {
						count++
					}
				}
			}
			has := func(xs []int) bool {
				for _, x := range xs {
					if x == b {
						return true
					}
				}
				return false
			}
			if has(got.Orphan) != (count == 0 && im.Used[b]) || has(got.Dangling) != (count > 0 && !im.Used[b]) || has(got.Duplicate) != (count > 1) {
				t.Fatal("classification", b, got)
			}
		}
		invalid := []int{}
		for _, in := range im.Inodes {
			for _, b := range in {
				if b < 5 || b >= 30 {
					invalid = append(invalid, b)
				}
			}
		}
		sort.Ints(invalid)
		if len(invalid) != len(got.Invalid) {
			t.Fatal("invalid count")
		}
		for i, x := range invalid {
			if got.Invalid[i] != x {
				t.Fatal("invalid")
			}
		}
		for _, xs := range [][]int{got.Orphan, got.Dangling, got.Duplicate} {
			for j := 1; j < len(xs); j++ {
				if xs[j] <= xs[j-1] {
					t.Fatal("order")
				}
			}
		}
	}
	for _, im := range []Image{{NBlocks: -1}, {NBlocks: 3, DataStart: 4, Used: make([]bool, 3)}, {NBlocks: 2, Used: nil}} {
		if !reflect.DeepEqual(Check(im), Report{BadGeometry: true}) {
			t.Fatal("geometry")
		}
	}
}
