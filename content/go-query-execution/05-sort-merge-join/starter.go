package main

import "fmt"

// ---- Provided: the shared vocabulary of the whole track -------------------

// Value is one cell. nil is SQL NULL; otherwise it is an int64 or a string.
// A column never mixes int64 and string values.
type Value = any

// Row is one tuple. Operators must never modify a row they received.
type Row []Value

// Stats collects the work counters the tests read. They count work, never time.
type Stats struct {
	RowsRead    int // rows handed out by a base-table scan
	Opens       int // times a base-table scan was opened
	Comparisons int // key comparisons (used where a lesson says so)
	Work        int // abstract work units (used by the planner lesson)
}

// Iterator is the pull ("Volcano") interface: Open prepares, Next returns the
// next row or false when exhausted, Close releases the input.
// Open may be called again after Close and must restart from the beginning.
type Iterator interface {
	Open()
	Next() (Row, bool)
	Close()
}

type Table struct {
	Name string
	Cols []string
	Rows []Row
}

// Collect opens an iterator, drains it and closes it.
func Collect(it Iterator) []Row {
	it.Open()
	defer it.Close()
	var out []Row
	for {
		r, ok := it.Next()
		if !ok {
			return out
		}
		out = append(out, r)
	}
}

// Tri is SQL three-valued logic. The numeric order False < Unknown < True is
// part of the contract.
type Tri int

const (
	False Tri = iota
	Unknown
	True
)

// Provided: a table scan. It counts every row it hands out in Stats.RowsRead
// and every Open in Stats.Opens.
type scan struct {
	t   *Table
	st  *Stats
	pos int
}

func NewScan(t *Table, st *Stats) Iterator { return &scan{t: t, st: st} }
func (s *scan) Open()                       { s.pos = 0; s.st.Opens++ }
func (s *scan) Close()                      {}
func (s *scan) Next() (Row, bool) {
	if s.pos >= len(s.t.Rows) {
		return nil, false
	}
	r := s.t.Rows[s.pos]
	s.pos++
	s.st.RowsRead++
	return r, true
}

// Provided: an equi-join predicate. NULL on either side is Unknown, and an
// int64 never equals a string.
func EqCols(li, ri int) func(l, r Row) Tri {
	return func(l, r Row) Tri {
		if l[li] == nil || r[ri] == nil {
			return Unknown
		}
		if l[li] == r[ri] {
			return True
		}
		return False
	}
}

func concatRows(a, b Row) Row {
	out := make(Row, 0, len(a)+len(b))
	return append(append(out, a...), b...)
}

// SortKey orders by one column. NULL is smaller than every value, so it comes
// first ascending and last descending.
type SortKey struct {
	Col  int
	Desc bool
}

// ---- Your work -----------------------------------------------------------

// NewSort reads all of in on the first Next, then returns the rows ordered by
// keys. Rows with equal keys keep their input order (stable).
func NewSort(in Iterator, keys []SortKey) Iterator {
	return nil
}

// NewMergeJoin joins two inputs that are ALREADY sorted ascending on their join
// column (NULLs first). One forward pass over each side, no rescanning.
func NewMergeJoin(left, right Iterator, lcol, rcol int) Iterator {
	return nil
}

func main() {
	a := &Table{Name: "a", Cols: []string{"k", "v"}, Rows: []Row{{int64(3), "c"}, {nil, "n"}, {int64(1), "a"}, {int64(3), "d"}}}
	b := &Table{Name: "b", Cols: []string{"k", "w"}, Rows: []Row{{int64(3), "x"}, {int64(1), "y"}, {int64(3), "z"}, {nil, "m"}}}
	var sa, sb Stats
	keys := []SortKey{{Col: 0}}
	j := NewMergeJoin(NewSort(NewScan(a, &sa), keys), NewSort(NewScan(b, &sb), keys), 0, 0)
	fmt.Println(Collect(j), "rows read:", sa.RowsRead+sb.RowsRead)
}
