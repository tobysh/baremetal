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

func xKeyed(seed int64, n, k, nullPct int) []Row {
	r := xrng(seed)
	rows := make([]Row, n)
	for i := range rows {
		rows[i] = Row{xInt(r, k, nullPct), fmt.Sprintf("p%d", i)}
	}
	return rows
}

func TestNLJ_matches_brute_force_with_duplicates_and_NULLs(t *testing.T) {
	for seed := int64(1); seed <= 30; seed++ {
		l := xKeyed(seed, int(seed%9)+1, 4, 25)
		r := xKeyed(seed*5, int(seed%7)+2, 4, 25)
		var sl, sr Stats
		got := Collect(NewNestedLoopJoin(NewScan(xTable("l", l, "k", "p"), &sl), NewScan(xTable("r", r, "k", "q"), &sr), EqCols(0, 0)))
		want := xNLRef(l, r, 0, 0)
		if !xSame(got, want) {
			t.Fatalf("seed %d: got %s; want %s", seed, xShow(got), xShow(want))
		}
	}
}

func TestNLJ_NULL_keys_never_match_each_other(t *testing.T) {
	l := []Row{{nil, "a"}, {nil, "b"}}
	r := []Row{{nil, "x"}}
	var s1, s2 Stats
	if got := Collect(NewNestedLoopJoin(NewScan(xTable("l", l, "k", "p"), &s1), NewScan(xTable("r", r, "k", "q"), &s2), EqCols(0, 0))); len(got) != 0 {
		t.Errorf("NULL = NULL joined: %s", xShow(got))
	}
}

func TestNLJ_duplicate_keys_multiply(t *testing.T) {
	l := []Row{{int64(1), "a"}, {int64(1), "b"}, {int64(1), "c"}}
	r := []Row{{int64(1), "x"}, {int64(1), "y"}, {int64(2), "z"}, {int64(1), "w"}}
	var s1, s2 Stats
	got := Collect(NewNestedLoopJoin(NewScan(xTable("l", l, "k", "p"), &s1), NewScan(xTable("r", r, "k", "q"), &s2), EqCols(0, 0)))
	if len(got) != 9 {
		t.Errorf("3 x 3 matching rows should give 9 results; got %d: %s", len(got), xShow(got))
	}
	if !xSame(got, xNLRef(l, r, 0, 0)) {
		t.Errorf("order wrong: got %s", xShow(got))
	}
}

func TestNLJ_theta_and_cross_joins(t *testing.T) {
	l := xKeyed(3, 6, 5, 20)
	r := xKeyed(4, 5, 5, 20)
	less := func(a, b Row) Tri {
		if a[0] == nil || b[0] == nil {
			return Unknown
		}
		if a[0].(int64) < b[0].(int64) {
			return True
		}
		return False
	}
	var want []Row
	for _, a := range l {
		for _, b := range r {
			if a[0] != nil && b[0] != nil && a[0].(int64) < b[0].(int64) {
				want = append(want, append(append(Row{}, a...), b...))
			}
		}
	}
	var s1, s2 Stats
	if got := Collect(NewNestedLoopJoin(NewScan(xTable("l", l), &s1), NewScan(xTable("r", r), &s2), less)); !xSame(got, want) {
		t.Errorf("a.k < b.k: got %s; want %s", xShow(got), xShow(want))
	}
	var s3, s4 Stats
	got := Collect(NewNestedLoopJoin(NewScan(xTable("l", l), &s3), NewScan(xTable("r", r), &s4), func(a, b Row) Tri { return True }))
	if len(got) != 30 {
		t.Errorf("cross join gave %d rows; want 30", len(got))
	}
}

func TestNLJ_rescans_the_inner_once_per_outer_row(t *testing.T) {
	l := xKeyed(7, 9, 3, 0)
	r := xKeyed(8, 6, 3, 0)
	var so, si Stats
	Collect(NewNestedLoopJoin(NewScan(xTable("l", l), &so), NewScan(xTable("r", r), &si), EqCols(0, 0)))
	if so.Opens != 1 || so.RowsRead != 9 {
		t.Errorf("outer: Opens=%d RowsRead=%d; want 1 and 9", so.Opens, so.RowsRead)
	}
	if si.Opens != 9 || si.RowsRead != 9*6 {
		t.Errorf("inner: Opens=%d RowsRead=%d; want 9 opens and 54 rows read (a nested loop scans the inner side once per outer row)", si.Opens, si.RowsRead)
	}
}

func TestNLJ_empty_sides(t *testing.T) {
	r := xKeyed(1, 5, 3, 0)
	var so, si Stats
	if got := Collect(NewNestedLoopJoin(NewScan(xTable("l", nil), &so), NewScan(xTable("r", r), &si), EqCols(0, 0))); len(got) != 0 {
		t.Errorf("empty outer produced rows")
	}
	if si.Opens != 0 || si.RowsRead != 0 {
		t.Errorf("empty outer: inner Opens=%d RowsRead=%d; the inner must not be touched without an outer row", si.Opens, si.RowsRead)
	}
	var s1, s2 Stats
	if got := Collect(NewNestedLoopJoin(NewScan(xTable("l", r), &s1), NewScan(xTable("r", nil), &s2), EqCols(0, 0))); len(got) != 0 {
		t.Errorf("empty inner produced rows")
	}
}

func TestNLJ_is_lazy_and_returns_fresh_rows(t *testing.T) {
	l := []Row{{int64(1), "a"}, {int64(2), "b"}, {int64(3), "c"}}
	r := []Row{{int64(9), "x"}, {int64(2), "y"}, {int64(2), "z"}}
	var so, si Stats
	j := NewNestedLoopJoin(NewScan(xTable("l", l), &so), NewScan(xTable("r", r), &si), EqCols(0, 0))
	j.Open()
	first, ok := j.Next()
	if !ok || xRow(first) != `(2, "b", 2, "y")` {
		t.Fatalf("first result = %v, %v", first, ok)
	}
	if so.RowsRead != 2 || si.RowsRead != 3+2 {
		t.Errorf("after the first result outer read %d (want 2) and inner read %d (want 5)", so.RowsRead, si.RowsRead)
	}
	first[0] = "scribble"
	if l[1][0] != int64(2) {
		t.Errorf("modifying a result changed the input row; build a new Row")
	}
	j.Close()
}
