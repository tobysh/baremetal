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

func xCeilLog2(n int) int {
	c := 0
	for 1<<c < n {
		c++
	}
	return c
}

// xRefRange: linear scan, then order by (key, rowid). NULL never matches a range.
func xRefRange(rows []Row, col int, lo, hi *Bound) []Row {
	type kr struct {
		k  int64
		id int
	}
	var m []kr
	for i, r := range rows {
		v, ok := r[col].(int64)
		if !ok {
			continue
		}
		if lo != nil && (v < lo.Val.(int64) || (v == lo.Val.(int64) && !lo.Inclusive)) {
			continue
		}
		if hi != nil && (v > hi.Val.(int64) || (v == hi.Val.(int64) && !hi.Inclusive)) {
			continue
		}
		m = append(m, kr{v, i})
	}
	sort.Slice(m, func(a, b int) bool { return m[a].k < m[b].k || (m[a].k == m[b].k && m[a].id < m[b].id) })
	var out []Row
	for _, x := range m {
		out = append(out, rows[x.id])
	}
	return out
}

func xTab6(seed int64, n, k, nullPct int) *Table {
	r := xrng(seed)
	rows := make([]Row, n)
	for i := range rows {
		rows[i] = Row{xInt(r, k, nullPct), fmt.Sprintf("r%d", i)}
	}
	return xTable("t", rows, "k", "v")
}

func TestBuildIndex_is_sorted_skips_NULL_and_orders_duplicates_by_rowid(t *testing.T) {
	tb := xTab6(3, 60, 8, 20)
	ix := BuildIndex(tb, 0)
	nonNull := 0
	for _, r := range tb.Rows {
		if r[0] != nil {
			nonNull++
		}
	}
	if len(ix.Entries) != nonNull {
		t.Fatalf("index has %d entries; want %d (NULL keys are not indexed)", len(ix.Entries), nonNull)
	}
	for i := 1; i < len(ix.Entries); i++ {
		a, b := ix.Entries[i-1], ix.Entries[i]
		if a.Key.(int64) > b.Key.(int64) || (a.Key == b.Key && a.RowID >= b.RowID) {
			t.Fatalf("entries %d,%d out of order: %v then %v", i-1, i, a, b)
		}
		if tb.Rows[b.RowID][0] != b.Key {
			t.Fatalf("entry %v points at the wrong row", b)
		}
	}
}

func TestIndexScan_matches_reference_for_all_bound_shapes(t *testing.T) {
	for seed := int64(1); seed <= 25; seed++ {
		tb := xTab6(seed, 40+int(seed), 10, 15)
		ix := BuildIndex(tb, 0)
		r := xrng(seed * 31)
		for q := 0; q < 12; q++ {
			var lo, hi *Bound
			if r.Intn(4) > 0 {
				lo = &Bound{int64(r.Intn(12) - 1), r.Intn(2) == 0}
			}
			if r.Intn(4) > 0 {
				hi = &Bound{int64(r.Intn(12) - 1), r.Intn(2) == 0}
			}
			var st Stats
			got := Collect(NewIndexScan(tb, ix, lo, hi, &st))
			want := xRefRange(tb.Rows, 0, lo, hi)
			if !xSame(got, want) {
				t.Fatalf("seed %d lo=%v hi=%v: got %s; want %s", seed, lo, hi, xShow(got), xShow(want))
			}
			if st.RowsRead != len(want) {
				t.Errorf("seed %d lo=%v hi=%v: RowsRead=%d; an index scan fetches only the %d matching rows", seed, lo, hi, st.RowsRead, len(want))
			}
		}
	}
}

func TestIndexScan_equality_on_a_duplicate_key_returns_every_row(t *testing.T) {
	tb := xTab6(7, 200, 4, 0)
	ix := BuildIndex(tb, 0)
	var st Stats
	b := &Bound{int64(2), true}
	got := Collect(NewIndexScan(tb, ix, b, b, &st))
	if want := xRefRange(tb.Rows, 0, b, b); !xSame(got, want) || len(got) < 20 {
		t.Errorf("got %d rows; want %d (%s)", len(got), len(want), xShow(got))
	}
	ex := &Bound{int64(2), false}
	if got := Collect(NewIndexScan(tb, ix, ex, ex, &Stats{})); len(got) != 0 {
		t.Errorf("(2, 2) exclusive returned rows")
	}
}

func TestIndexScan_exclusive_lower_bound_skips_all_duplicates(t *testing.T) {
	tb := xTab6(8, 100, 3, 0)
	ix := BuildIndex(tb, 0)
	lo := &Bound{int64(1), false}
	got := Collect(NewIndexScan(tb, ix, lo, nil, &Stats{}))
	if want := xRefRange(tb.Rows, 0, lo, nil); !xSame(got, want) {
		t.Errorf("k > 1: got %d rows, want %d", len(got), len(want))
	}
}

func TestIndexScan_unbounded_scan_omits_NULL_rows(t *testing.T) {
	tb := xTab6(9, 50, 6, 40)
	ix := BuildIndex(tb, 0)
	got := Collect(NewIndexScan(tb, ix, nil, nil, &Stats{}))
	want := xRefRange(tb.Rows, 0, nil, nil)
	if !xSame(got, want) {
		t.Errorf("unbounded index scan returned %d rows; want the %d non-NULL rows in key order", len(got), len(want))
	}
	if len(got) == len(tb.Rows) {
		t.Fatalf("test table had no NULLs")
	}
}

func TestIndexScan_comparison_budget_is_logarithmic(t *testing.T) {
	for _, n := range []int{1, 2, 10, 100, 1000, 5000} {
		tb := xTab6(int64(n), n, n/2+1, 0)
		ix := BuildIndex(tb, 0)
		ix.Compares = 0
		lo := &Bound{int64(n / 4), true}
		hi := &Bound{int64(n/4 + 1), true}
		var st Stats
		got := Collect(NewIndexScan(tb, ix, lo, hi, &st))
		budget := xCeilLog2(len(ix.Entries)+1) + 1 + len(got) + 1
		if ix.Compares > budget {
			t.Errorf("n=%d: %d key comparisons for %d matches; budget is %d (binary search to the start, then one comparison per row walked plus one to stop). A linear search from the start of the index is too slow.", n, ix.Compares, len(got), budget)
		}
		if ix.Compares == 0 {
			t.Errorf("n=%d: no comparisons were counted; use idx.Cmp for every key comparison", n)
		}
	}
}

func TestIndexScan_string_keys_and_empty_table(t *testing.T) {
	tb := xTable("t", []Row{{"pear"}, {"apple"}, {nil}, {"fig"}, {"apple"}}, "k")
	ix := BuildIndex(tb, 0)
	got := Collect(NewIndexScan(tb, ix, &Bound{"b", true}, &Bound{"g", true}, &Stats{}))
	if len(got) != 1 || got[0][0] != "fig" {
		t.Errorf("b <= k <= g gave %s; want fig", xShow(got))
	}
	empty := xTable("e", nil, "k")
	if got := Collect(NewIndexScan(empty, BuildIndex(empty, 0), nil, nil, &Stats{})); len(got) != 0 {
		t.Errorf("empty table returned rows")
	}
}

func TestIndexScan_reopen_and_exhaustion(t *testing.T) {
	tb := xTab6(2, 30, 5, 0)
	ix := BuildIndex(tb, 0)
	var st Stats
	s := NewIndexScan(tb, ix, &Bound{int64(1), true}, &Bound{int64(2), true}, &st)
	a := Collect(s)
	ix.Compares = 0
	s.Open()
	for {
		if _, ok := s.Next(); !ok {
			break
		}
	}
	before := ix.Compares
	for i := 0; i < 3; i++ {
		s.Next()
	}
	if ix.Compares != before {
		t.Errorf("Next after exhaustion keeps comparing keys")
	}
	s.Close()
	if b := Collect(s); !xSame(a, b) || st.Opens != 3 {
		t.Errorf("reopen: second run differs or Opens=%d (want 3)", st.Opens)
	}
}
