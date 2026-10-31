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

func xHJ(l, r []Row, sl, sr *Stats) []Row {
	return Collect(NewHashJoin(NewScan(xTable("l", l, "k", "p"), sl), NewScan(xTable("r", r, "k", "q"), sr), 0, 0))
}

func TestHashJoin_matches_brute_force_including_order(t *testing.T) {
	for seed := int64(1); seed <= 40; seed++ {
		l := xKeyed(seed, int(seed%13), 5, 20)
		r := xKeyed(seed*3, int(seed%11), 5, 20)
		var sl, sr Stats
		got, want := xHJ(l, r, &sl, &sr), xNLRef(l, r, 0, 0)
		if !xSame(got, want) {
			t.Fatalf("seed %d: got %s; want %s (left order, then right insertion order)", seed, xShow(got), xShow(want))
		}
	}
}

func TestHashJoin_reads_each_side_exactly_once(t *testing.T) {
	l := xKeyed(1, 200, 7, 10)
	r := xKeyed(2, 150, 7, 10)
	var sl, sr Stats
	xHJ(l, r, &sl, &sr)
	if sl.Opens != 1 || sr.Opens != 1 || sl.RowsRead != 200 || sr.RowsRead != 150 {
		t.Errorf("left Opens=%d RowsRead=%d, right Opens=%d RowsRead=%d; want 1/200 and 1/150. A hash join never rescans.", sl.Opens, sl.RowsRead, sr.Opens, sr.RowsRead)
	}
}

func TestHashJoin_heavy_duplicates(t *testing.T) {
	l := xKeyed(5, 60, 3, 0)
	r := xKeyed(6, 40, 3, 0)
	var sl, sr Stats
	got := xHJ(l, r, &sl, &sr)
	if want := xNLRef(l, r, 0, 0); !xSame(got, want) {
		t.Errorf("got %d rows, want %d; first got %s", len(got), len(want), xShow(got))
	}
}

func TestHashJoin_NULL_keys_are_dropped_on_both_sides(t *testing.T) {
	l := []Row{{nil, "a"}, {int64(1), "b"}}
	r := []Row{{nil, "x"}, {int64(1), "y"}, {nil, "z"}}
	var sl, sr Stats
	got := xHJ(l, r, &sl, &sr)
	if len(got) != 1 || xRow(got[0]) != `(1, "b", 1, "y")` {
		t.Errorf("got %s; want only the key-1 pair", xShow(got))
	}
}

func TestHashJoin_keys_of_different_types_never_match(t *testing.T) {
	l := []Row{{int64(1), "a"}, {"1", "b"}, {"x", "c"}}
	r := []Row{{"1", "p"}, {int64(1), "q"}, {int64(0), "r"}}
	var sl, sr Stats
	got := xHJ(l, r, &sl, &sr)
	want := []Row{{int64(1), "a", int64(1), "q"}, {"1", "b", "1", "p"}}
	if !xSame(got, want) {
		t.Errorf("got %s; want %s. int64(1) and the string \"1\" are different keys.", xShow(got), xShow(want))
	}
}

func TestHashJoin_other_key_columns(t *testing.T) {
	l := []Row{{"pad", int64(2)}, {"pen", int64(9)}, {"ink", int64(1)}}
	r := []Row{{int64(1), "ada"}, {int64(2), "bo"}, {int64(2), "bo2"}}
	var sl, sr Stats
	got := Collect(NewHashJoin(NewScan(xTable("l", l), &sl), NewScan(xTable("r", r), &sr), 1, 0))
	if want := xNLRef(l, r, 1, 0); !xSame(got, want) {
		t.Errorf("got %s; want %s", xShow(got), xShow(want))
	}
}

func TestHashJoin_empty_inputs_and_build_timing(t *testing.T) {
	r := xKeyed(3, 12, 3, 0)
	var sl, sr Stats
	if got := xHJ(nil, r, &sl, &sr); len(got) != 0 {
		t.Errorf("empty left gave rows")
	}
	if sr.RowsRead != 12 {
		t.Errorf("empty left: right RowsRead=%d; the build side is read in full on the first Next, so 12", sr.RowsRead)
	}
	var s3, s4 Stats
	l := xKeyed(4, 5, 3, 0)
	j := NewHashJoin(NewScan(xTable("l", l), &s3), NewScan(xTable("r", r), &s4), 0, 0)
	j.Open()
	if s3.RowsRead != 0 || s4.RowsRead != 0 {
		t.Errorf("Open read rows; work must wait until the first Next")
	}
	j.Close()
	var s5, s6 Stats
	if got := xHJ(l, nil, &s5, &s6); len(got) != 0 || s5.RowsRead != 5 {
		t.Errorf("empty right: got %d rows, left read %d", len(got), s5.RowsRead)
	}
}

func TestHashJoin_streams_the_probe_side(t *testing.T) {
	l := xKeyed(8, 100, 2, 0)
	r := xKeyed(9, 10, 2, 0)
	var sl, sr Stats
	j := NewHashJoin(NewScan(xTable("l", l), &sl), NewScan(xTable("r", r), &sr), 0, 0)
	j.Open()
	j.Next()
	if sl.RowsRead != 1 {
		t.Errorf("after one result the left side was read %d times; want 1 (stream the probe side)", sl.RowsRead)
	}
	j.Close()
}

func TestHashJoin_returns_fresh_rows(t *testing.T) {
	l := []Row{{int64(1), "a"}}
	r := []Row{{int64(1), "x"}}
	var sl, sr Stats
	got := xHJ(l, r, &sl, &sr)
	got[0][0] = "scribble"
	if l[0][0] != int64(1) || r[0][0] != int64(1) {
		t.Errorf("modifying the result changed an input row")
	}
}
