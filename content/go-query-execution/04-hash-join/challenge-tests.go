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

func xLOJ(l, r []Row, w int) []Row {
	var out []Row
	for _, a := range l {
		n := 0
		for _, b := range r {
			if a[0] != nil && b[0] != nil && a[0] == b[0] {
				out = append(out, append(append(Row{}, a...), b...))
				n++
			}
		}
		if n == 0 {
			out = append(out, append(append(Row{}, a...), make(Row, w)...))
		}
	}
	return out
}

func TestOuter_matches_reference(t *testing.T) {
	for seed := int64(1); seed <= 40; seed++ {
		l := xKeyed(seed, int(seed%12), 5, 25)
		r := xKeyed(seed*7, int(seed%9), 5, 25)
		var sl, sr Stats
		got := Collect(NewHashLeftOuterJoin(NewScan(xTable("l", l), &sl), NewScan(xTable("r", r), &sr), 0, 0, 2))
		if want := xLOJ(l, r, 2); !xSame(got, want) {
			t.Fatalf("seed %d: got %s; want %s", seed, xShow(got), xShow(want))
		}
		if sl.RowsRead != len(l) || sr.RowsRead != len(r) || sl.Opens != 1 || sr.Opens != 1 {
			t.Errorf("seed %d: not one pass per side (left %d/%d, right %d/%d)", seed, sl.RowsRead, sl.Opens, sr.RowsRead, sr.Opens)
		}
	}
}

func TestOuter_NULL_left_key_is_padded_not_dropped(t *testing.T) {
	l := []Row{{nil, "a"}}
	r := []Row{{nil, "x"}}
	var sl, sr Stats
	got := Collect(NewHashLeftOuterJoin(NewScan(xTable("l", l), &sl), NewScan(xTable("r", r), &sr), 0, 0, 2))
	if len(got) != 1 || xRow(got[0]) != `(NULL, "a", NULL, NULL)` {
		t.Errorf("got %s; want (NULL, \"a\", NULL, NULL)", xShow(got))
	}
}

func TestOuter_empty_right_pads_every_row_using_rightWidth(t *testing.T) {
	l := xKeyed(3, 4, 3, 0)
	var sl, sr Stats
	got := Collect(NewHashLeftOuterJoin(NewScan(xTable("l", l), &sl), NewScan(xTable("r", nil), &sr), 0, 0, 3))
	if len(got) != 4 || len(got[0]) != 5 {
		t.Errorf("got %s; want 4 rows of width 5", xShow(got))
	}
}

func TestOuter_padding_rows_are_independent(t *testing.T) {
	l := []Row{{int64(1), "a"}, {int64(2), "b"}}
	var sl, sr Stats
	got := Collect(NewHashLeftOuterJoin(NewScan(xTable("l", l), &sl), NewScan(xTable("r", nil), &sr), 0, 0, 1))
	got[0][2] = "scribble"
	if got[1][2] != nil {
		t.Errorf("padding rows share storage")
	}
}
