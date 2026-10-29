package main

import (
	"reflect"
	"strings"
	"testing"
)

// The reference implementation; the learner's Clamp is never trusted.
func refClamp(x, lo, hi int) int {
	if lo > hi {
		lo, hi = hi, lo
	}
	if x < lo {
		return lo
	}
	if x > hi {
		return hi
	}
	return x
}

var mutants = map[string]func(x, lo, hi int) int{
	"ignores the upper bound": func(x, lo, hi int) int {
		if lo > hi {
			lo, hi = hi, lo
		}
		if x < lo {
			return lo
		}
		return x
	},
	"ignores the lower bound": func(x, lo, hi int) int {
		if lo > hi {
			lo, hi = hi, lo
		}
		if x > hi {
			return hi
		}
		return x
	},
	"never swaps the bounds": func(x, lo, hi int) int {
		if x < lo {
			return lo
		}
		if x > hi {
			return hi
		}
		return x
	},
	"treats a negative x as below the range": func(x, lo, hi int) int {
		if lo > hi {
			lo, hi = hi, lo
		}
		if x < 0 && x >= lo {
			return lo
		}
		if x < lo {
			return lo
		}
		if x > hi {
			return hi
		}
		return x
	},
	"breaks when lo == hi": func(x, lo, hi int) int {
		if lo == hi && x != lo {
			return 0
		}
		return refClamp(x, lo, hi)
	},
	"off by one at the top edge": func(x, lo, hi int) int {
		r := refClamp(x, lo, hi)
		if x == hi && lo < hi {
			return r - 1
		}
		return r
	},
	"off by one at the bottom edge": func(x, lo, hi int) int {
		r := refClamp(x, lo, hi)
		if x == lo && lo < hi {
			return r + 1
		}
		return r
	},
	"returns hi when above, even for swapped bounds": func(x, lo, hi int) int {
		if x > hi && lo > hi {
			return lo + 1
		}
		return refClamp(x, lo, hi)
	},
}

func TestClampCases_are_well_formed(t *testing.T) {
	cases := ClampCases()
	if len(cases) < 5 {
		t.Fatalf("only %d cases; a useful table needs more", len(cases))
	}
	seen := map[string]bool{}
	for i, c := range cases {
		if strings.TrimSpace(c.Name) == "" {
			t.Errorf("case %d has an empty Name", i)
		}
		if seen[c.Name] {
			t.Errorf("name %q is used twice; subtest names must be unique", c.Name)
		}
		seen[c.Name] = true
	}
}

func TestClampCases_pass_against_the_correct_function(t *testing.T) {
	for _, c := range ClampCases() {
		if got := refClamp(c.X, c.Lo, c.Hi); got != c.Want {
			t.Errorf("case %q is wrong: Clamp(%d, %d, %d) is %d, not %d", c.Name, c.X, c.Lo, c.Hi, got, c.Want)
		}
	}
}

func TestClampCases_catch_every_mutant(t *testing.T) {
	cases := ClampCases()
	if len(cases) == 0 {
		t.Fatal("no cases")
	}
	for name, m := range mutants {
		caught := false
		for _, c := range cases {
			if m(c.X, c.Lo, c.Hi) != c.Want {
				caught = true
				break
			}
		}
		if !caught {
			t.Errorf("a broken Clamp that %s passes your whole table; add a case that fails it", name)
		}
	}
}

func TestCheckClamp_reports_failures_in_order(t *testing.T) {
	cases := []ClampCase{
		{"ok", 5, 1, 10, 5},
		{"wrong want", 5, 1, 10, 6},
		{"also fine", 0, 1, 10, 1},
		{"bad again", 20, 1, 10, 11},
	}
	got := CheckClamp(refClamp, cases)
	want := []string{
		"wrong want: Clamp(5, 1, 10) = 5; want 6",
		"bad again: Clamp(20, 1, 10) = 10; want 11",
	}
	if !reflect.DeepEqual(got, want) {
		t.Errorf("CheckClamp =\n  %q\nwant\n  %q", got, want)
	}
}

func TestCheckClamp_all_pass_is_empty(t *testing.T) {
	if got := CheckClamp(refClamp, []ClampCase{{"a", 1, 0, 2, 1}}); len(got) != 0 {
		t.Errorf("CheckClamp = %q; want none", got)
	}
	if got := CheckClamp(refClamp, nil); len(got) != 0 {
		t.Errorf("CheckClamp(nil) = %q; want none", got)
	}
}

func TestCheckClamp_uses_the_function_it_is_given(t *testing.T) {
	always7 := func(x, lo, hi int) int { return 7 }
	got := CheckClamp(always7, []ClampCase{{"seven", 1, 0, 9, 7}, {"one", 1, 0, 9, 1}})
	if len(got) != 1 || !strings.HasPrefix(got[0], "one: ") {
		t.Errorf("CheckClamp = %q; want exactly the failing case \"one\"", got)
	}
}

func TestCheckClamp_on_learner_table_with_mutants(t *testing.T) {
	cases := ClampCases()
	for name, m := range mutants {
		if fails := CheckClamp(m, cases); len(fails) == 0 {
			t.Errorf("CheckClamp found nothing wrong with the mutant that %s", name)
		}
	}
	if fails := CheckClamp(refClamp, cases); len(fails) != 0 {
		t.Errorf("CheckClamp on the correct function reported %q", fails)
	}
}
