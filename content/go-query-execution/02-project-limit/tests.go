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

type xSpy struct {
	in             Iterator
	opens, closes  int
	pulls          int
}

func (s *xSpy) Open()  { s.opens++; s.in.Open() }
func (s *xSpy) Close() { s.closes++; s.in.Close() }
func (s *xSpy) Next() (Row, bool) {
	s.pulls++
	return s.in.Next()
}

func xTab(seed int64, n int) *Table {
	r := xrng(seed)
	rows := make([]Row, n)
	for i := range rows {
		rows[i] = Row{int64(i), xInt(r, 50, 30), xInt(r, 9, 30), fmt.Sprintf("n%d", r.Intn(5))}
	}
	return xTable("t", rows, "id", "a", "b", "s")
}

func TestAdd_and_Concat_propagate_NULL(t *testing.T) {
	cases := []struct {
		a, b Value
		want Value
	}{
		{int64(2), int64(3), int64(5)}, {int64(-4), int64(4), int64(0)}, {nil, int64(3), nil}, {int64(3), nil, nil}, {nil, nil, nil},
	}
	for _, c := range cases {
		if got := Add(Const(c.a), Const(c.b))(nil); got != c.want {
			t.Errorf("Add(%s, %s) = %s; want %s", xCell(c.a), xCell(c.b), xCell(got), xCell(c.want))
		}
	}
	sc := []struct{ a, b, want Value }{{"ab", "cd", "abcd"}, {"", "x", "x"}, {nil, "x", nil}, {"x", nil, nil}}
	for _, c := range sc {
		if got := Concat(Const(c.a), Const(c.b))(nil); got != c.want {
			t.Errorf("Concat(%s, %s) = %s; want %s", xCell(c.a), xCell(c.b), xCell(got), xCell(c.want))
		}
	}
}

func TestProject_matches_reference(t *testing.T) {
	for seed := int64(1); seed <= 20; seed++ {
		tb := xTab(seed, int(seed*3))
		var want []Row
		for _, r := range tb.Rows {
			var sum Value
			if r[1] != nil && r[2] != nil {
				sum = r[1].(int64) + r[2].(int64)
			}
			want = append(want, Row{r[3], sum, "k", r[0]})
		}
		var st Stats
		got := Collect(NewProject(NewScan(tb, &st), []Expr{Col(3), Add(Col(1), Col(2)), Const("k"), Col(0)}))
		if !xSame(got, want) {
			t.Fatalf("seed %d: got %s; want %s", seed, xShow(got), xShow(want))
		}
		if st.RowsRead != len(tb.Rows) {
			t.Errorf("seed %d: RowsRead=%d; want %d", seed, st.RowsRead, len(tb.Rows))
		}
	}
}

func TestProject_returns_fresh_rows(t *testing.T) {
	tb := xTab(5, 6)
	var st Stats
	got := Collect(NewProject(NewScan(tb, &st), []Expr{Col(0), Col(1)}))
	for _, r := range got {
		r[0] = "scribble"
	}
	if tb.Rows[0][0] != int64(0) {
		t.Errorf("editing a projected row changed the table's row; Project must allocate a new Row per output")
	}
	if len(got) > 1 && &got[0][0] == &got[1][0] {
		t.Errorf("two output rows share one backing array")
	}
}

func TestProject_passes_Open_and_Close_down(t *testing.T) {
	spy := &xSpy{in: NewScan(xTab(1, 4), &Stats{})}
	Collect(NewProject(spy, []Expr{Col(0)}))
	if spy.opens != 1 || spy.closes != 1 {
		t.Errorf("child opened %d and closed %d times; want 1 and 1", spy.opens, spy.closes)
	}
}

func TestLimit_matches_reference_for_many_offsets(t *testing.T) {
	tb := xTab(9, 12)
	for off := 0; off <= 14; off++ {
		for n := 0; n <= 14; n += 2 {
			lo := off
			if lo > 12 {
				lo = 12
			}
			hi := lo + n
			if hi > 12 {
				hi = 12
			}
			want := tb.Rows[lo:hi]
			var st Stats
			got := Collect(NewLimit(NewScan(tb, &st), off, n))
			if !xSame(got, want) {
				t.Fatalf("offset %d limit %d: got %s; want %s", off, n, xShow(got), xShow(want))
			}
		}
	}
}

func TestLimit_stops_pulling(t *testing.T) {
	tb := xTab(2, 100)
	cases := []struct{ off, n, wantRead int }{{0, 3, 3}, {10, 5, 15}, {0, 0, 0}, {95, 10, 100}, {200, 3, 100}}
	for _, c := range cases {
		var st Stats
		got := Collect(NewLimit(NewScan(tb, &st), c.off, c.n))
		if st.RowsRead != c.wantRead {
			t.Errorf("offset %d limit %d: RowsRead = %d; want %d (got %d rows)", c.off, c.n, st.RowsRead, c.wantRead, len(got))
		}
	}
}

func TestLimit_does_not_pull_after_it_is_satisfied(t *testing.T) {
	spy := &xSpy{in: NewScan(xTab(2, 20), &Stats{})}
	l := NewLimit(spy, 0, 2)
	l.Open()
	l.Next()
	l.Next()
	for i := 0; i < 3; i++ {
		if _, ok := l.Next(); ok {
			t.Fatalf("a third row came out of Limit 2")
		}
	}
	if spy.pulls != 2 {
		t.Errorf("child was pulled %d times; want exactly 2", spy.pulls)
	}
	l.Close()
}

func TestLimit_restarts_on_reopen(t *testing.T) {
	tb := xTab(4, 10)
	var st Stats
	l := NewLimit(NewScan(tb, &st), 2, 3)
	a, b := Collect(l), Collect(l)
	if !xSame(a, b) || len(a) != 3 {
		t.Errorf("second run %s differs from first %s", xShow(b), xShow(a))
	}
}

func TestPipeline_filter_project_limit_is_lazy(t *testing.T) {
	tb := xTab(11, 80)
	var st Stats
	pipe := NewLimit(NewProject(NewFilter(NewScan(tb, &st), func(r Row) bool { return r[2] != nil && r[2].(int64) < 3 }),
		[]Expr{Col(0), Col(2)}), 0, 4)
	got := Collect(pipe)
	var want []Row
	read := 0
	for _, r := range tb.Rows {
		read++
		if r[2] != nil && r[2].(int64) < 3 {
			want = append(want, Row{r[0], r[2]})
			if len(want) == 4 {
				break
			}
		}
	}
	if !xSame(got, want) {
		t.Errorf("got %s; want %s", xShow(got), xShow(want))
	}
	if st.RowsRead != read {
		t.Errorf("RowsRead = %d; the fourth match is row number %d, so exactly that many should be read", st.RowsRead, read)
	}
}
