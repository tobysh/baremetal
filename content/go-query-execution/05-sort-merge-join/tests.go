package main

import (
	"fmt"
	"math/rand"
	"reflect"
	"sort"
	"testing"
)

// ---- test helpers (names start with x so they never clash with yours) ----

var _ = reflect.DeepEqual
var _ = sort.Ints
var _ = fmt.Sprint

func xrng(seed int64) *rand.Rand { return rand.New(rand.NewSource(seed)) }

// xInt: NULL with probability nullPct percent, else an int64 in [0,k).
func xInt(r *rand.Rand, k, nullPct int) Value {
	if r.Intn(100) < nullPct {
		return nil
	}
	return int64(r.Intn(k))
}

func xCell(v Value) string {
	switch x := v.(type) {
	case nil:
		return "NULL"
	case int64:
		return fmt.Sprintf("%d", x)
	case string:
		return fmt.Sprintf("%q", x)
	}
	return fmt.Sprintf("?%v", v)
}

func xRow(r Row) string {
	s := "("
	for i, v := range r {
		if i > 0 {
			s += ", "
		}
		s += xCell(v)
	}
	return s + ")"
}

func xRows(rs []Row) []string {
	out := make([]string, len(rs))
	for i, r := range rs {
		out[i] = xRow(r)
	}
	return out
}

func xShow(rs []Row) string {
	s := xRows(rs)
	if len(s) > 6 {
		return fmt.Sprint(s[:6]) + fmt.Sprintf(" ... (%d rows)", len(s))
	}
	return fmt.Sprint(s)
}

// xSame compares row lists in order (nil and empty are equal).
func xSame(a, b []Row) bool {
	if len(a) != len(b) {
		return false
	}
	x, y := xRows(a), xRows(b)
	for i := range x {
		if x[i] != y[i] {
			return false
		}
	}
	return true
}

// xSameSet compares row lists as multisets.
func xSameSet(a, b []Row) bool {
	x, y := xRows(a), xRows(b)
	sort.Strings(x)
	sort.Strings(y)
	return reflect.DeepEqual(append([]string{}, x...), append([]string{}, y...))
}

func xTable(name string, rows []Row, cols ...string) *Table {
	return &Table{Name: name, Cols: cols, Rows: rows}
}

// xNLRef is the brute-force reference equi-join: left order, then right order,
// NULL keys never match, int64 never equals string.
func xNLRef(l, r []Row, lc, rc int) []Row {
	var out []Row
	for _, a := range l {
		for _, b := range r {
			if a[lc] == nil || b[rc] == nil {
				continue
			}
			if a[lc] == b[rc] {
				out = append(out, append(append(Row{}, a...), b...))
			}
		}
	}
	return out
}

// xRefSort: insertion sort (stable) with an independent comparator. NULL is the smallest value.
func xRank(v Value) (int, int64, string) {
	switch x := v.(type) {
	case nil:
		return 0, 0, ""
	case int64:
		return 1, x, ""
	case string:
		return 1, 0, x
	}
	return 2, 0, ""
}

func xLess(a, b Value) bool {
	ra, ia, sa := xRank(a)
	rb, ib, sb := xRank(b)
	if ra != rb {
		return ra < rb
	}
	return ia < ib || (ia == ib && sa < sb)
}

func xRefSort(rows []Row, keys []SortKey) []Row {
	out := append([]Row{}, rows...)
	before := func(a, b Row) bool { // strictly before
		for _, k := range keys {
			x, y := a[k.Col], b[k.Col]
			if k.Desc {
				x, y = y, x
			}
			if xLess(x, y) {
				return true
			}
			if xLess(y, x) {
				return false
			}
		}
		return false
	}
	for i := 1; i < len(out); i++ {
		for j := i; j > 0 && before(out[j], out[j-1]); j-- {
			out[j], out[j-1] = out[j-1], out[j]
		}
	}
	return out
}

func xKeyed3(seed int64, n, k, nullPct int) []Row {
	r := xrng(seed)
	rows := make([]Row, n)
	for i := range rows {
		rows[i] = Row{xInt(r, k, nullPct), xInt(r, 3, 20), fmt.Sprintf("p%d", i)}
	}
	return rows
}

func TestSort_matches_reference_stable_with_NULLs(t *testing.T) {
	keySets := [][]SortKey{{{0, false}}, {{0, true}}, {{1, false}, {0, true}}, {{0, false}, {1, false}}}
	for seed := int64(1); seed <= 30; seed++ {
		rows := xKeyed3(seed, int(seed*4%50), 6, 25)
		keys := keySets[seed%4]
		var st Stats
		got := Collect(NewSort(NewScan(xTable("t", rows), &st), keys))
		if want := xRefSort(rows, keys); !xSame(got, want) {
			t.Fatalf("seed %d keys %v: got %s; want %s", seed, keys, xShow(got), xShow(want))
		}
		if st.RowsRead != len(rows) || st.Opens != 1 {
			t.Errorf("seed %d: RowsRead=%d Opens=%d", seed, st.RowsRead, st.Opens)
		}
	}
}

func TestSort_strings_and_NULL_placement(t *testing.T) {
	rows := []Row{{"b"}, {nil}, {"a"}, {"ab"}, {nil}}
	var st Stats
	if got := Collect(NewSort(NewScan(xTable("t", rows), &st), []SortKey{{0, false}})); xRow(got[0]) != "(NULL)" || xRow(got[1]) != "(NULL)" || xRow(got[4]) != `("b")` {
		t.Errorf("ascending: %s; NULLs come first", xShow(got))
	}
	var st2 Stats
	if got := Collect(NewSort(NewScan(xTable("t", rows), &st2), []SortKey{{0, true}})); xRow(got[0]) != `("b")` || xRow(got[4]) != "(NULL)" {
		t.Errorf("descending: %s; NULLs come last", xShow(got))
	}
}

func TestSort_is_blocking_and_reopens(t *testing.T) {
	rows := xKeyed3(1, 20, 5, 0)
	var st Stats
	s := NewSort(NewScan(xTable("t", rows), &st), []SortKey{{0, false}})
	s.Open()
	if st.RowsRead != 0 {
		t.Errorf("Open read rows")
	}
	s.Next()
	if st.RowsRead != 20 {
		t.Errorf("after the first Next RowsRead = %d; a sort must read everything before returning anything", st.RowsRead)
	}
	s.Close()
	if got := Collect(s); len(got) != 20 {
		t.Errorf("reopened sort returned %d rows; want 20", len(got))
	}
}

func xMJ(l, r []Row, lc, rc int, sl, sr *Stats) []Row {
	kl, kr := []SortKey{{lc, false}}, []SortKey{{rc, false}}
	return Collect(NewMergeJoin(
		NewSort(NewScan(xTable("l", l), sl), kl), NewSort(NewScan(xTable("r", r), sr), kr), lc, rc))
}

func TestMergeJoin_matches_brute_force(t *testing.T) {
	for seed := int64(1); seed <= 40; seed++ {
		l := xKeyed3(seed, int(seed%14), 4, 25)
		r := xKeyed3(seed*9, int(seed%10), 4, 25)
		var sl, sr Stats
		got := xMJ(l, r, 0, 0, &sl, &sr)
		// brute force over the sorted inputs: same order contract
		want := xNLRef(xRefSort(l, []SortKey{{0, false}}), xRefSort(r, []SortKey{{0, false}}), 0, 0)
		if !xSame(got, want) {
			t.Fatalf("seed %d: got %s; want %s", seed, xShow(got), xShow(want))
		}
	}
}

func TestMergeJoin_duplicate_groups_and_NULLs(t *testing.T) {
	l := []Row{{nil, "n1"}, {int64(1), "a"}, {int64(1), "b"}, {int64(1), "c"}, {int64(2), "d"}, {int64(4), "e"}}
	r := []Row{{nil, "m"}, {int64(1), "x"}, {int64(1), "y"}, {int64(3), "z"}, {int64(4), "w"}, {int64(4), "v"}}
	var sl, sr Stats
	got := Collect(NewMergeJoin(NewScan(xTable("l", l), &sl), NewScan(xTable("r", r), &sr), 0, 0))
	if len(got) != 3*2+1*2 {
		t.Errorf("got %d rows; want 8 (a 3x2 group and a 1x2 group): %s", len(got), xShow(got))
	}
	if want := xNLRef(l, r, 0, 0); !xSame(got, want) {
		t.Errorf("got %s; want %s", xShow(got), xShow(want))
	}
}

func TestMergeJoin_makes_one_pass_over_each_side(t *testing.T) {
	l := xKeyed3(5, 300, 10, 10)
	r := xKeyed3(6, 200, 10, 10)
	sorted := func(rows []Row) []Row { return xRefSort(rows, []SortKey{{0, false}}) }
	var sl, sr Stats
	got := Collect(NewMergeJoin(NewScan(xTable("l", sorted(l)), &sl), NewScan(xTable("r", sorted(r)), &sr), 0, 0))
	if !xSame(got, xNLRef(sorted(l), sorted(r), 0, 0)) {
		t.Fatalf("wrong result")
	}
	if sl.Opens != 1 || sr.Opens != 1 || sl.RowsRead > len(l) || sr.RowsRead > len(r) {
		t.Errorf("left %d reads/%d opens, right %d reads/%d opens; want at most 300 and 200 reads and 1 open each (no rescanning)", sl.RowsRead, sl.Opens, sr.RowsRead, sr.Opens)
	}
}

func TestMergeJoin_stops_when_one_side_ends(t *testing.T) {
	l := []Row{{int64(1), "a"}}
	r := make([]Row, 100)
	for i := range r {
		r[i] = Row{int64(i + 5), "x"}
	}
	var sl, sr Stats
	got := Collect(NewMergeJoin(NewScan(xTable("l", l), &sl), NewScan(xTable("r", r), &sr), 0, 0))
	if len(got) != 0 {
		t.Fatalf("expected no result")
	}
	if sr.RowsRead > 2 {
		t.Errorf("right was read %d times; the first right row (5) already exceeds the only left key (1), so at most 2 reads", sr.RowsRead)
	}
}

func TestMergeJoin_empty_sides_and_reopen(t *testing.T) {
	r := xKeyed3(1, 5, 3, 0)
	var s1, s2 Stats
	if got := xMJ(nil, r, 0, 0, &s1, &s2); len(got) != 0 {
		t.Errorf("empty left joined")
	}
	if got := xMJ(r, nil, 0, 0, &s1, &s2); len(got) != 0 {
		t.Errorf("empty right joined")
	}
	var a, b Stats
	j := NewMergeJoin(NewSort(NewScan(xTable("l", r), &a), []SortKey{{0, false}}), NewSort(NewScan(xTable("r", r), &b), []SortKey{{0, false}}), 0, 0)
	x, y := Collect(j), Collect(j)
	if !xSame(x, y) || len(x) == 0 {
		t.Errorf("a second run differs: %s vs %s", xShow(x), xShow(y))
	}
}
