package main

import (
	"fmt"
	"sort"
)

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

func cmpValue(a, b Value) int {
	switch {
	case a == nil && b == nil:
		return 0
	case a == nil:
		return -1
	case b == nil:
		return 1
	}
	switch x := a.(type) {
	case int64:
		y := b.(int64)
		if x < y {
			return -1
		} else if x > y {
			return 1
		}
	case string:
		y := b.(string)
		if x < y {
			return -1
		} else if x > y {
			return 1
		}
	}
	return 0
}

type sorter struct {
	in   Iterator
	keys []SortKey
	rows []Row
	pos  int
	done bool
}

func NewSort(in Iterator, keys []SortKey) Iterator { return &sorter{in: in, keys: keys} }
func (s *sorter) Open()                            { s.in.Open(); s.rows, s.pos, s.done = nil, 0, false }
func (s *sorter) Close()                           { s.in.Close() }
func (s *sorter) Next() (Row, bool) {
	if !s.done {
		s.done = true
		for {
			r, ok := s.in.Next()
			if !ok {
				break
			}
			s.rows = append(s.rows, r)
		}
		sort.SliceStable(s.rows, func(i, j int) bool {
			for _, k := range s.keys {
				c := cmpValue(s.rows[i][k.Col], s.rows[j][k.Col])
				if k.Desc {
					c = -c
				}
				if c != 0 {
					return c < 0
				}
			}
			return false
		})
	}
	if s.pos >= len(s.rows) {
		return nil, false
	}
	s.pos++
	return s.rows[s.pos-1], true
}

type mergeJoin struct {
	left, right Iterator
	lc, rc      int
	started     bool
	l           Row
	lok         bool
	r           Row
	rok         bool
	group       []Row
	groupKey    Value
	gi          int
}

func NewMergeJoin(left, right Iterator, lcol, rcol int) Iterator {
	return &mergeJoin{left: left, right: right, lc: lcol, rc: rcol}
}
func (j *mergeJoin) Open() {
	j.left.Open()
	j.right.Open()
	j.started, j.group, j.gi, j.groupKey = false, nil, 0, nil
}
func (j *mergeJoin) Close() { j.left.Close(); j.right.Close() }
func (j *mergeJoin) Next() (Row, bool) {
	if !j.started {
		j.started = true
		j.l, j.lok = j.left.Next()
		j.r, j.rok = j.right.Next()
	}
	for {
		// emit pending pairs of the current left row against the buffered group
		if j.gi < len(j.group) {
			j.gi++
			return concatRows(j.l, j.group[j.gi-1]), true
		}
		if j.group != nil && j.lok {
			// advance left; reuse the group if the next left row has the same key
			prev := j.l
			j.l, j.lok = j.left.Next()
			if j.lok && j.l[j.lc] != nil && cmpValue(j.l[j.lc], prev[j.lc]) == 0 {
				j.gi = 0
				continue
			}
			j.group = nil
		}
		if !j.lok {
			return nil, false
		}
		if j.l[j.lc] == nil {
			j.l, j.lok = j.left.Next()
			continue
		}
		for j.rok && (j.r[j.rc] == nil || cmpValue(j.r[j.rc], j.l[j.lc]) < 0) {
			j.r, j.rok = j.right.Next()
		}
		if !j.rok {
			return nil, false
		}
		if cmpValue(j.r[j.rc], j.l[j.lc]) > 0 {
			j.l, j.lok = j.left.Next()
			continue
		}
		j.group = []Row{}
		key := j.r[j.rc]
		for j.rok && j.r[j.rc] != nil && cmpValue(j.r[j.rc], key) == 0 {
			j.group = append(j.group, j.r)
			j.r, j.rok = j.right.Next()
		}
		j.gi = 0
	}
}

func main() {
	a := &Table{Name: "a", Cols: []string{"k", "v"}, Rows: []Row{{int64(3), "c"}, {nil, "n"}, {int64(1), "a"}, {int64(3), "d"}}}
	b := &Table{Name: "b", Cols: []string{"k", "w"}, Rows: []Row{{int64(3), "x"}, {int64(1), "y"}, {int64(3), "z"}, {nil, "m"}}}
	var sa, sb Stats
	keys := []SortKey{{Col: 0}}
	j := NewMergeJoin(NewSort(NewScan(a, &sa), keys), NewSort(NewScan(b, &sb), keys), 0, 0)
	fmt.Println(Collect(j), "rows read:", sa.RowsRead+sb.RowsRead)
}
