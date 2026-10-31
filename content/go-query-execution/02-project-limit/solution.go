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

// Provided: a filter with a plain bool predicate (NULL handling is lesson 1's job).
type filter struct {
	in   Iterator
	pred func(Row) bool
}

func NewFilter(in Iterator, pred func(Row) bool) Iterator { return &filter{in, pred} }
func (f *filter) Open()                                    { f.in.Open() }
func (f *filter) Close()                                   { f.in.Close() }
func (f *filter) Next() (Row, bool) {
	for {
		r, ok := f.in.Next()
		if !ok {
			return nil, false
		}
		if f.pred(r) {
			return r, true
		}
	}
}

// Expr computes one output cell from an input row.
type Expr func(Row) Value

func Col(i int) Expr      { return func(r Row) Value { return r[i] } }
func Const(v Value) Expr  { return func(Row) Value { return v } }

func Add(a, b Expr) Expr {
	return func(r Row) Value {
		x, y := a(r), b(r)
		if x == nil || y == nil {
			return nil
		}
		return x.(int64) + y.(int64)
	}
}

func Concat(a, b Expr) Expr {
	return func(r Row) Value {
		x, y := a(r), b(r)
		if x == nil || y == nil {
			return nil
		}
		return x.(string) + y.(string)
	}
}

type project struct {
	in    Iterator
	exprs []Expr
}

func NewProject(in Iterator, exprs []Expr) Iterator { return &project{in, exprs} }
func (p *project) Open()                            { p.in.Open() }
func (p *project) Close()                           { p.in.Close() }
func (p *project) Next() (Row, bool) {
	r, ok := p.in.Next()
	if !ok {
		return nil, false
	}
	out := make(Row, len(p.exprs))
	for i, e := range p.exprs {
		out[i] = e(r)
	}
	return out, true
}

type limit struct {
	in      Iterator
	offset  int
	n       int
	skipped bool
	given   int
}

func NewLimit(in Iterator, offset, n int) Iterator { return &limit{in: in, offset: offset, n: n} }
func (l *limit) Open() {
	l.in.Open()
	l.skipped, l.given = false, 0
}
func (l *limit) Close() { l.in.Close() }
func (l *limit) Next() (Row, bool) {
	if l.given >= l.n {
		return nil, false
	}
	if !l.skipped {
		l.skipped = true
		for i := 0; i < l.offset; i++ {
			if _, ok := l.in.Next(); !ok {
				l.given = l.n
				return nil, false
			}
		}
	}
	r, ok := l.in.Next()
	if !ok {
		l.given = l.n
		return nil, false
	}
	l.given++
	return r, true
}

func main() {
	t := &Table{Name: "t", Cols: []string{"id", "price", "tax"}, Rows: []Row{
		{int64(1), int64(100), int64(8)}, {int64(2), int64(50), nil}, {int64(3), int64(70), int64(5)}, {int64(4), int64(10), int64(1)},
	}}
	var st Stats
	q := NewLimit(NewProject(NewScan(t, &st), []Expr{Col(0), Add(Col(1), Col(2))}), 1, 2)
	fmt.Println(Collect(q), "rows read:", st.RowsRead)
}
