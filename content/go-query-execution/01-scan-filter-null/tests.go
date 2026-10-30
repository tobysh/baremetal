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

func TestAnd_Or_Not_truth_tables(t *testing.T) {
	// Independent reference: with False=0, Unknown=1, True=2, AND is min, OR is max, NOT is 2-x.
	for a := Tri(0); a <= 2; a++ {
		for b := Tri(0); b <= 2; b++ {
			wa, wo := a, b
			if b < a {
				wa = b
			}
			if b < a {
				wo = a
			}
			if got := And(a, b); got != wa {
				t.Errorf("And(%d,%d) = %d; want %d (0=False 1=Unknown 2=True)", a, b, got, wa)
			}
			if got := Or(a, b); got != wo {
				t.Errorf("Or(%d,%d) = %d; want %d", a, b, got, wo)
			}
		}
		if got := Not(a); got != 2-a {
			t.Errorf("Not(%d) = %d; want %d", a, got, 2-a)
		}
	}
}

var xAllowed = map[string][3]bool{ // indexed by sign(a-b)+1
	"=": {false, true, false}, "!=": {true, false, true},
	"<": {true, false, false}, "<=": {true, true, false},
	">": {false, false, true}, ">=": {false, true, true},
}

func xRefCompare(op string, a, b Value) Tri {
	if a == nil || b == nil {
		return Unknown
	}
	sign := 0
	switch x := a.(type) {
	case int64:
		y, ok := b.(int64)
		if !ok {
			return Unknown
		}
		sign = int((x-y)>>63) | int((y-x)>>63)*-1
		if x == y {
			sign = 0
		} else if x < y {
			sign = -1
		} else {
			sign = 1
		}
	case string:
		y, ok := b.(string)
		if !ok {
			return Unknown
		}
		sign = 0
		if x < y {
			sign = -1
		} else if x > y {
			sign = 1
		}
	}
	if xAllowed[op][sign+1] {
		return True
	}
	return False
}

func TestCompare_with_NULL_is_Unknown_for_every_operator(t *testing.T) {
	for op := range xAllowed {
		for _, c := range [][2]Value{{nil, nil}, {nil, int64(3)}, {int64(3), nil}, {nil, "x"}, {"x", nil}} {
			if got := Compare(op, c[0], c[1]); got != Unknown {
				t.Errorf("Compare(%q, %v, %v) = %d; any comparison with NULL, even NULL = NULL, is Unknown (1)", op, c[0], c[1], got)
			}
		}
	}
}

func TestCompare_matches_the_reference(t *testing.T) {
	vals := []Value{int64(-2), int64(0), int64(5), int64(5), int64(9), "", "a", "b", "ab", nil}
	for op := range xAllowed {
		for _, a := range vals {
			for _, b := range vals {
				if got, want := Compare(op, a, b), xRefCompare(op, a, b); got != want {
					t.Errorf("Compare(%q, %s, %s) = %d; want %d", op, xCell(a), xCell(b), got, want)
				}
			}
		}
	}
}

func xRandomTable(seed int64, n int) *Table {
	r := xrng(seed)
	rows := make([]Row, n)
	for i := range rows {
		rows[i] = Row{int64(i), xInt(r, 6, 25), xInt(r, 4, 25)}
	}
	return xTable("t", rows, "id", "a", "b")
}

func TestScan_returns_every_row_in_order_and_counts_them(t *testing.T) {
	for _, n := range []int{0, 1, 7, 60} {
		tb := xRandomTable(int64(n), n)
		var st Stats
		got := Collect(NewScan(tb, &st))
		if !xSame(got, tb.Rows) {
			t.Errorf("n=%d: scan returned %s; want the table's rows in order", n, xShow(got))
		}
		if st.RowsRead != n || st.Opens != 1 {
			t.Errorf("n=%d: RowsRead=%d Opens=%d; want %d and 1", n, st.RowsRead, st.Opens, n)
		}
	}
}

func TestScan_stays_exhausted_and_can_be_reopened(t *testing.T) {
	tb := xRandomTable(3, 5)
	var st Stats
	it := NewScan(tb, &st)
	it.Open()
	for i := 0; i < 5; i++ {
		it.Next()
	}
	for i := 0; i < 3; i++ {
		if r, ok := it.Next(); ok || r != nil {
			t.Fatalf("Next after the end returned %v, %v; want nil, false", r, ok)
		}
	}
	it.Close()
	if st.RowsRead != 5 {
		t.Errorf("RowsRead = %d after reading 5 rows and probing past the end; want 5", st.RowsRead)
	}
	again := Collect(it)
	if !xSame(again, tb.Rows) || st.Opens != 2 || st.RowsRead != 10 {
		t.Errorf("reopen: got %s, Opens=%d, RowsRead=%d; want all rows again, 2, 10", xShow(again), st.Opens, st.RowsRead)
	}
}

// Reference filter: keeps rows whose condition is exactly True, using the reference comparison.
func xRefFilter(rows []Row, keep func(Row) Tri) []Row {
	var out []Row
	for _, r := range rows {
		if keep(r) == True {
			out = append(out, r)
		}
	}
	return out
}

func TestFilter_keeps_only_True_not_Unknown(t *testing.T) {
	tb := xTable("t", []Row{{int64(1), int64(10)}, {int64(2), nil}, {int64(3), int64(30)}, {int64(4), nil}}, "id", "v")
	var st Stats
	got := Collect(NewFilter(NewScan(tb, &st), func(r Row) Tri { return Compare(">", r[1], int64(5)) }))
	if want := []Row{tb.Rows[0], tb.Rows[2]}; !xSame(got, want) {
		t.Errorf("v > 5 returned %s; want rows 1 and 3 (NULL > 5 is Unknown and is dropped)", xShow(got))
	}
}

func TestFilter_NOT_of_Unknown_is_still_dropped(t *testing.T) {
	tb := xTable("t", []Row{{int64(1), int64(10)}, {int64(2), nil}, {int64(3), int64(1)}}, "id", "v")
	var st Stats
	got := Collect(NewFilter(NewScan(tb, &st), func(r Row) Tri { return Not(Compare(">", r[1], int64(5))) }))
	if want := []Row{tb.Rows[2]}; !xSame(got, want) {
		t.Errorf("NOT (v > 5) returned %s; want only id 3. The NULL row is neither > 5 nor NOT > 5.", xShow(got))
	}
}

func TestFilter_matches_the_reference_on_random_predicates(t *testing.T) {
	ops := []string{"=", "!=", "<", "<=", ">", ">="}
	for seed := int64(1); seed <= 40; seed++ {
		r := xrng(seed * 77)
		tb := xRandomTable(seed, 30+r.Intn(60))
		op1, op2 := ops[r.Intn(6)], ops[r.Intn(6)]
		c1, c2 := int64(r.Intn(6)), int64(r.Intn(4))
		shape := r.Intn(4)
		mk := func(cmp func(string, Value, Value) Tri, and, or func(a, b Tri) Tri, not func(Tri) Tri) func(Row) Tri {
			return func(row Row) Tri {
				x, y := cmp(op1, row[1], c1), cmp(op2, row[2], c2)
				switch shape {
				case 0:
					return and(x, y)
				case 1:
					return or(x, y)
				case 2:
					return not(and(x, not(y)))
				}
				return and(not(x), or(y, x))
			}
		}
		refAnd := func(a, b Tri) Tri {
			if b < a {
				return b
			}
			return a
		}
		refOr := func(a, b Tri) Tri {
			if b > a {
				return b
			}
			return a
		}
		refNot := func(a Tri) Tri { return 2 - a }
		want := xRefFilter(tb.Rows, mk(xRefCompare, refAnd, refOr, refNot))
		var st Stats
		got := Collect(NewFilter(NewScan(tb, &st), mk(Compare, And, Or, Not)))
		if !xSame(got, want) {
			t.Fatalf("seed %d (shape %d, %q %d, %q %d): got %s; want %s", seed, shape, op1, c1, op2, c2, xShow(got), xShow(want))
		}
		if st.RowsRead != len(tb.Rows) {
			t.Errorf("seed %d: RowsRead = %d; a full drain reads each of the %d rows once", seed, st.RowsRead, len(tb.Rows))
		}
	}
}

func TestFilter_is_lazy(t *testing.T) {
	rows := make([]Row, 50)
	for i := range rows {
		rows[i] = Row{int64(i)}
	}
	tb := xTable("t", rows, "id")
	var st Stats
	f := NewFilter(NewScan(tb, &st), func(r Row) Tri { return Compare("=", r[0], int64(7)) })
	f.Open()
	r, ok := f.Next()
	if !ok || r[0] != int64(7) {
		t.Fatalf("first Next = %v, %v; want row 7", r, ok)
	}
	if st.RowsRead != 8 {
		t.Errorf("after the first result RowsRead = %d; a lazy filter reads rows 0..7 only, so 8", st.RowsRead)
	}
	if _, ok := f.Next(); ok {
		t.Errorf("a second Next returned a row; only row 7 matches")
	}
	if st.RowsRead != 50 {
		t.Errorf("after exhausting RowsRead = %d; want 50", st.RowsRead)
	}
}

func TestFilter_all_NULL_column_returns_nothing_for_any_comparison(t *testing.T) {
	rows := []Row{{nil}, {nil}, {nil}}
	tb := xTable("t", rows, "v")
	for op := range xAllowed {
		var st Stats
		for _, p := range []func(Row) Tri{
			func(r Row) Tri { return Compare(op, r[0], int64(1)) },
			func(r Row) Tri { return Not(Compare(op, r[0], int64(1))) },
		} {
			if got := Collect(NewFilter(NewScan(tb, &st), p)); len(got) != 0 {
				t.Errorf("op %q over all-NULL rows returned %s; want nothing", op, xShow(got))
			}
		}
	}
}
