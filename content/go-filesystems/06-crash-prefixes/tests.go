package main

import (
	"math/rand"
	"reflect"
	"testing"
)

func cuts(t *testing.T, name string, initial State, writes []Write, want ...int) {
	t.Helper()
	got := UnsafeCuts(initial, writes)
	if len(got) == 0 && len(want) == 0 {
		return
	}
	if !reflect.DeepEqual(got, want) {
		t.Fatalf("%s: UnsafeCuts = %v, want %v", name, got, want)
	}
}

func TestLessonExample(t *testing.T) {
	init := State{Used: []bool{true, false, false}, Ptr: 0}
	cuts(t, "safe order", init, []Write{{"bit", 1, 1}, {"ptr", 0, 1}, {"bit", 0, 0}})
	cuts(t, "free first", init, []Write{{"bit", 0, 0}, {"ptr", 0, 1}, {"bit", 1, 1}}, 1, 2)
	cuts(t, "point first", init, []Write{{"ptr", 0, 1}, {"bit", 1, 1}, {"bit", 0, 0}}, 1)
}

func TestNoWritesAndEmptyPointer(t *testing.T) {
	cuts(t, "no writes", State{Used: []bool{true, false}, Ptr: 0}, nil)
	cuts(t, "empty pointer never unsafe", State{Used: []bool{false, false}, Ptr: -1}, []Write{{"bit", 0, 1}, {"bit", 0, 0}, {"bit", 1, 1}})
	cuts(t, "clearing the pointer is safe", State{Used: []bool{true}, Ptr: 0}, []Write{{"ptr", 0, -1}, {"bit", 0, 0}})
}

func TestFinalAndInitialCutsAreChecked(t *testing.T) {
	// Only the last cut (all writes applied) is unsafe: the final write frees the pointed-to block.
	cuts(t, "last cut", State{Used: []bool{true, true}, Ptr: 1}, []Write{{"bit", 0, 0}, {"bit", 1, 0}}, 2)
	// Unsafe states can appear and disappear between cuts.
	cuts(t, "unsafe in the middle only", State{Used: []bool{true, true}, Ptr: 0}, []Write{{"bit", 0, 0}, {"bit", 0, 1}, {"bit", 1, 0}}, 1)
}

func TestPointerMovesAndRepeats(t *testing.T) {
	init := State{Used: []bool{true, false, true, false}, Ptr: 0}
	ws := []Write{
		{"ptr", 0, 1},  // cut1 -> ptr 1 (free): unsafe
		{"bit", 1, 1},  // cut2 -> ok
		{"ptr", 0, 3},  // cut3 -> ptr 3 (free): unsafe
		{"ptr", 7, 2},  // cut4 -> ptr 2 (used), Block ignored: ok
		{"bit", 2, 0},  // cut5 -> ptr 2 now free: unsafe
		{"bit", 2, 1},  // cut6 -> ok
		{"ptr", 0, -1}, // cut7 -> ok
	}
	cuts(t, "moving pointer", init, ws, 1, 3, 5)
}

func TestIncreasingOrderAndLength(t *testing.T) {
	init := State{Used: []bool{false, false, false, false}, Ptr: -1}
	ws := []Write{}
	for i := 0; i < 20; i++ {
		ws = append(ws, Write{"ptr", 0, i % 4}) // never allocated: unsafe on every cut after the first
	}
	got := UnsafeCuts(init, ws)
	if len(got) != 20 || got[0] != 1 || got[19] != 20 {
		t.Fatalf("got %v, want cuts 1..20 (cut 0 is the initial state, cut 20 the last)", got)
	}
	for i := 1; i < len(got); i++ {
		if got[i] <= got[i-1] {
			t.Fatalf("cuts must be strictly increasing: %v", got)
		}
	}
}

func TestInputsNotMutated(t *testing.T) {
	used := []bool{true, false, true, false}
	init := State{Used: used, Ptr: 0}
	ws := []Write{{"bit", 0, 0}, {"bit", 1, 1}, {"ptr", 0, 1}, {"bit", 3, 1}}
	wsCopy := append([]Write(nil), ws...)
	UnsafeCuts(init, ws)
	UnsafeCuts(init, ws)
	if !reflect.DeepEqual(used, []bool{true, false, true, false}) || init.Ptr != 0 {
		t.Fatal("UnsafeCuts modified the initial state")
	}
	if !reflect.DeepEqual(ws, wsCopy) {
		t.Fatal("UnsafeCuts modified the writes")
	}
}

func TestCutsAgainstBruteForce(t *testing.T) {
	r := rand.New(rand.NewSource(91))
	for n := 0; n < 400; n++ {
		size := 2 + n%5
		used := make([]bool, size)
		for i := range used {
			used[i] = r.Intn(2) == 0
		}
		ptr := -1
		if r.Intn(3) > 0 {
			ptr = r.Intn(size)
			used[ptr] = true // the initial state is consistent
		}
		initial := State{Used: append([]bool(nil), used...), Ptr: ptr}
		writes := []Write{}
		for i := 0; i < n%17; i++ {
			if r.Intn(2) == 0 {
				writes = append(writes, Write{"ptr", r.Intn(size), r.Intn(size+1) - 1})
			} else {
				writes = append(writes, Write{"bit", r.Intn(size), r.Intn(2)})
			}
		}
		want := []int{}
		for cut := 0; cut <= len(writes); cut++ {
			bits := append([]bool(nil), used...)
			p := ptr
			for _, w := range writes[:cut] {
				if w.Kind == "ptr" {
					p = w.Value
				} else {
					bits[w.Block] = w.Value == 1
				}
			}
			if p >= 0 && !bits[p] {
				want = append(want, cut)
			}
		}
		got := UnsafeCuts(initial, writes)
		if len(got) != len(want) || (len(want) > 0 && !reflect.DeepEqual(got, want)) {
			t.Fatalf("case %d: initial %+v writes %v\n got %v\nwant %v", n, initial, writes, got, want)
		}
	}
}
