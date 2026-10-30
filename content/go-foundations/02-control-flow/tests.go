package main

import "testing"

func TestSumTo_values(t *testing.T) {
	for _, c := range []struct{ n, want int }{{1, 1}, {4, 10}, {10, 55}, {100, 5050}, {0, 0}, {-5, 0}} {
		if got := SumTo(c.n); got != c.want {
			t.Errorf("SumTo(%d) = %d; want %d", c.n, got, c.want)
		}
	}
}

func TestCollatz_values(t *testing.T) {
	for _, c := range []struct{ n, want int }{{1, 0}, {2, 1}, {3, 7}, {6, 8}, {7, 16}, {27, 111}} {
		if got := Collatz(c.n); got != c.want {
			t.Errorf("Collatz(%d) = %d; want %d", c.n, got, c.want)
		}
	}
}

func TestDayKind_values(t *testing.T) {
	for _, c := range []struct{ in, want string }{
		{"sat", "weekend"}, {"sun", "weekend"}, {"mon", "weekday"}, {"wed", "weekday"},
		{"fri", "weekday"}, {"", "unknown"}, {"Mon", "unknown"}, {"holiday", "unknown"},
	} {
		if got := DayKind(c.in); got != c.want {
			t.Errorf("DayKind(%q) = %q; want %q", c.in, got, c.want)
		}
	}
}

func TestIndexOf_finds_first(t *testing.T) {
	xs := []int{5, 7, 7, 9}
	for _, c := range []struct{ target, want int }{{5, 0}, {7, 1}, {9, 3}, {4, -1}} {
		if got := IndexOf(xs, c.target); got != c.want {
			t.Errorf("IndexOf(%v, %d) = %d; want %d", xs, c.target, got, c.want)
		}
	}
}

func TestIndexOf_empty(t *testing.T) {
	if got := IndexOf(nil, 1); got != -1 {
		t.Errorf("IndexOf(nil, 1) = %d; want -1", got)
	}
}

func TestSumEvensUntil_values(t *testing.T) {
	for _, c := range []struct {
		xs    []int
		limit int
		want  int
	}{
		{[]int{2, 3, 4, 5, 6}, 100, 12},
		{[]int{2, 4, 50, 6}, 10, 6},
		{[]int{1, 3, 5}, 10, 0},
		{[]int{-4, -3, -2}, 0, -6},
		{[]int{8, 12, 2}, 8, 8},
		{nil, 5, 0},
	} {
		if got := SumEvensUntil(c.xs, c.limit); got != c.want {
			t.Errorf("SumEvensUntil(%v, %d) = %d; want %d", c.xs, c.limit, got, c.want)
		}
	}
}
