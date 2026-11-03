package main

import (
	"math/rand"
	"testing"
)

func TestPlanner(t *testing.T) {
	r := rand.New(rand.NewSource(18))
	for i := 0; i < 3000; i++ {
		s := Stats{L: int64(r.Intn(1000001)), R: int64(r.Intn(1000001)), Matches: int64(r.Intn(8)), Selected: int64(r.Intn(100)), SortedLeft: r.Intn(2) == 0, SortedRight: r.Intn(2) == 0, IndexLeft: r.Intn(2) == 0, IndexRight: r.Intn(2) == 0}
		if i < 10 {
			s.L = int64(i % 3)
			s.R = int64(i % 2)
		}
		names := []string{"nested", "hash", "merge", "index"}
		merge := s.L + s.R
		if !s.SortedLeft {
			merge += 4 * s.L
		}
		if !s.SortedRight {
			merge += 4 * s.R
		}
		costs := []int64{s.L * s.R, 3 * (s.L + s.R), merge, s.L * (2 + s.Matches)}
		best := 0
		limit := 3
		if s.IndexRight {
			limit = 4
		}
		for j := 1; j < limit; j++ {
			if costs[j] < costs[best] {
				best = j
			}
		}
		access := "scan"
		if s.IndexLeft && 2+s.Selected < s.L {
			access = "index"
		}
		want := Plan{names[best], access, costs[best]}
		if got := Choose(s); got != want {
			t.Fatal(s, got, want)
		}
	}
}
