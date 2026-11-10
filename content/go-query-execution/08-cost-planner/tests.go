package main

import (
	"math/rand"
	"testing"
)

// xref is an independent reference: a table of candidates scanned in tie order.
func xref(s Stats) Plan {
	merge := s.L + s.R
	if !s.SortedLeft {
		merge += 4 * s.L
	}
	if !s.SortedRight {
		merge += 4 * s.R
	}
	type cand struct {
		name string
		cost int64
		ok   bool
	}
	cands := []cand{
		{"nested", s.L * s.R, true},
		{"hash", 3 * (s.L + s.R), true},
		{"merge", merge, true},
		{"index", s.L * (2 + s.Matches), s.IndexRight},
	}
	var best *cand
	for i := range cands {
		if cands[i].ok && (best == nil || cands[i].cost < best.cost) {
			best = &cands[i]
		}
	}
	access := "scan"
	if s.IndexLeft && 2+s.Selected < s.L {
		access = "index"
	}
	return Plan{best.name, access, best.cost}
}

func xexpect(t *testing.T, what string, s Stats, join, access string, cost int64) {
	t.Helper()
	got := Choose(s)
	want := Plan{join, access, cost}
	if got != want {
		t.Fatalf("%s: Choose(%+v) = %+v, want %+v", what, s, got, want)
	}
}

func TestPlanner_lesson_example_picks_the_index_join(t *testing.T) {
	xexpect(t, "lesson example", Stats{L: 1000, R: 2000, IndexRight: true, Matches: 1}, "index", "scan", 3000)
	// without the index, hash (9000) beats merge (15000) and nested
	xexpect(t, "same sizes, no index", Stats{L: 1000, R: 2000, Matches: 1}, "hash", "scan", 9000)
}

func TestPlanner_each_strategy_wins_somewhere(t *testing.T) {
	xexpect(t, "tiny inputs", Stats{L: 2, R: 3}, "nested", "scan", 6)
	xexpect(t, "big unsorted", Stats{L: 5000, R: 4000}, "hash", "scan", 27000)
	xexpect(t, "both sorted", Stats{L: 5000, R: 4000, SortedLeft: true, SortedRight: true}, "merge", "scan", 9000)
	xexpect(t, "indexed, few matches", Stats{L: 10, R: 1000000, IndexRight: true, Matches: 1}, "index", "scan", 30)
}

func TestPlanner_ties_go_to_the_earlier_candidate(t *testing.T) {
	xexpect(t, "nested = hash (6*6 = 3*12)", Stats{L: 6, R: 6}, "nested", "scan", 36)
	xexpect(t, "hash = merge (left sorted, right not)", Stats{L: 10, R: 10, SortedLeft: true}, "hash", "scan", 60)
	xexpect(t, "merge = index (30 = 10*3)", Stats{L: 10, R: 20, SortedLeft: true, SortedRight: true, IndexRight: true, Matches: 1}, "merge", "scan", 30)
	xexpect(t, "nested = index (3000 = 1000*3)", Stats{L: 1000, R: 3, IndexRight: true, Matches: 1}, "nested", "scan", 3000)
	xexpect(t, "everything is 0 when both tables are empty", Stats{IndexRight: true}, "nested", "scan", 0)
}

func TestPlanner_index_join_needs_IndexRight(t *testing.T) {
	// the index formula would give 100*2 = 200, far below merge's 100100
	both := Stats{L: 100, R: 100000, Matches: 0, SortedLeft: true, SortedRight: true}
	xexpect(t, "no right index", both, "merge", "scan", 100100)
	both.IndexRight = true
	xexpect(t, "with right index", both, "index", "scan", 200)
}

func TestPlanner_empty_tables(t *testing.T) {
	xexpect(t, "empty left", Stats{L: 0, R: 500, IndexRight: true, Matches: 3}, "nested", "scan", 0)
	xexpect(t, "empty right", Stats{L: 500, R: 0}, "nested", "scan", 0)
}

func TestPlanner_million_row_inputs_do_not_overflow(t *testing.T) {
	xexpect(t, "sorted million x million", Stats{L: 1000000, R: 1000000, SortedLeft: true, SortedRight: true}, "merge", "scan", 2000000)
	xexpect(t, "unsorted million x million", Stats{L: 1000000, R: 1000000}, "hash", "scan", 6000000)
	xexpect(t, "million, index with 7 matches", Stats{L: 1000000, R: 1000000, IndexRight: true, Matches: 7}, "hash", "scan", 6000000)
	xexpect(t, "million x 2 nested", Stats{L: 1000000, R: 2}, "nested", "scan", 2000000)
}

func TestPlanner_access_path_is_independent_and_has_a_strict_boundary(t *testing.T) {
	// 2+Selected < L : strict
	xexpect(t, "selected = L-3 uses the index", Stats{L: 100, R: 100, IndexLeft: true, Selected: 97}, "hash", "index", 600)
	xexpect(t, "selected = L-2 does not (2+97+1 = 100, not < 100)", Stats{L: 100, R: 100, IndexLeft: true, Selected: 98}, "hash", "scan", 600)
	xexpect(t, "no left index, tiny selection", Stats{L: 100, R: 100, Selected: 0}, "hash", "scan", 600)
	xexpect(t, "empty left", Stats{L: 0, R: 5, IndexLeft: true}, "nested", "scan", 0)
}

func TestPlanner_matches_reference_on_random_statistics(t *testing.T) {
	r := rand.New(rand.NewSource(18))
	for i := 0; i < 3000; i++ {
		s := Stats{L: int64(r.Intn(1000001)), R: int64(r.Intn(1000001)), Matches: int64(r.Intn(8)), Selected: int64(r.Intn(100)),
			SortedLeft: r.Intn(2) == 0, SortedRight: r.Intn(2) == 0, IndexLeft: r.Intn(2) == 0, IndexRight: r.Intn(2) == 0}
		switch i % 4 { // also small tables, where ties and nested joins matter
		case 1:
			s.L, s.R = int64(r.Intn(12)), int64(r.Intn(12))
		case 2:
			s.L = int64(r.Intn(60))
			s.R = int64(r.Intn(60))
		}
		if i < 10 {
			s.L = int64(i % 3)
			s.R = int64(i % 2)
		}
		if got, want := Choose(s), xref(s); got != want {
			t.Fatalf("Choose(%+v) = %+v, want %+v", s, got, want)
		}
	}
}
