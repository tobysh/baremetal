package main

import (
	"math/rand"
	"reflect"
	"testing"
)

func TestCuts(t *testing.T) {
	r := rand.New(rand.NewSource(91))
	for n := 0; n < 300; n++ {
		initial := State{Used: []bool{true, false, false, true}, Ptr: 0}
		writes := []Write{}
		for i := 0; i < n%17; i++ {
			if r.Intn(2) == 0 {
				writes = append(writes, Write{"ptr", 0, r.Intn(5) - 1})
			} else {
				writes = append(writes, Write{"bit", r.Intn(4), r.Intn(2)})
			}
		}
		want := []int{}
		for cut := 0; cut <= len(writes); cut++ {
			bits := [4]bool{true, false, false, true}
			ptr := 0
			for _, w := range writes[:cut] {
				if w.Kind == "ptr" {
					ptr = w.Value
				} else {
					bits[w.Block] = w.Value == 1
				}
			}
			if ptr >= 0 && !bits[ptr] {
				want = append(want, cut)
			}
		}
		got := UnsafeCuts(initial, writes)
		if len(got) != len(want) {
			t.Fatal(got, want)
		}
		for i := range got {
			if got[i] != want[i] {
				t.Fatal(got, want)
			}
		}
		if !reflect.DeepEqual(initial.Used, []bool{true, false, false, true}) {
			t.Fatal("mutated")
		}
	}
}
