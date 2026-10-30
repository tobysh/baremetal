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

// ---- Your work -----------------------------------------------------------

func And(a, b Tri) Tri {
	return Unknown
}

func Or(a, b Tri) Tri {
	return Unknown
}

func Not(a Tri) Tri {
	return Unknown
}

// Compare evaluates "a op b" for op in =, !=, <, <=, >, >=.
func Compare(op string, a, b Value) Tri {
	return Unknown
}

type scan struct {
	t   *Table
	st  *Stats
	pos int
}

func NewScan(t *Table, st *Stats) Iterator { return &scan{t: t, st: st} }
func (s *scan) Open()                       {}
func (s *scan) Close()                      {}
func (s *scan) Next() (Row, bool)           { return nil, false }

func NewFilter(in Iterator, pred func(Row) Tri) Iterator {
	return nil
}

func main() {
	t := &Table{Name: "t", Cols: []string{"id", "score"}, Rows: []Row{
		{int64(1), int64(40)}, {int64(2), nil}, {int64(3), int64(90)},
	}}
	var st Stats
	over50 := NewFilter(NewScan(t, &st), func(r Row) Tri { return Compare(">", r[1], int64(50)) })
	fmt.Println(Collect(over50), "rows read:", st.RowsRead)
	fmt.Println("NOT (score > 50) for NULL:", Not(Compare(">", nil, int64(50))))
}
