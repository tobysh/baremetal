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

// Entry is one index record: a non-NULL key and the position of its row.
type Entry struct {
	Key   Value
	RowID int
}

// Index is a sorted list of entries (by key, then RowID). NULL keys are NOT
// indexed. Every key comparison you make MUST go through Cmp, which counts
// them in Compares.
type Index struct {
	Entries  []Entry
	Compares int
}

// Cmp returns -1, 0 or 1 for two non-NULL keys of the same type.
func (ix *Index) Cmp(a, b Value) int {
	ix.Compares++
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

// Bound is one end of a key range; a nil *Bound means unbounded.
type Bound struct {
	Val       Value
	Inclusive bool
}

func BuildIndex(t *Table, col int) *Index {
	ix := &Index{}
	for i, r := range t.Rows {
		if r[col] != nil {
			ix.Entries = append(ix.Entries, Entry{r[col], i})
		}
	}
	sort.SliceStable(ix.Entries, func(a, b int) bool { return ix.Cmp(ix.Entries[a].Key, ix.Entries[b].Key) < 0 })
	return ix
}

type indexScan struct {
	t      *Table
	ix     *Index
	lo, hi *Bound
	st     *Stats
	pos    int
	done   bool
}

func NewIndexScan(t *Table, idx *Index, lo, hi *Bound, st *Stats) Iterator {
	return &indexScan{t: t, ix: idx, lo: lo, hi: hi, st: st}
}
func (s *indexScan) Close() {}
func (s *indexScan) Open() {
	s.st.Opens++
	s.done = false
	s.pos = 0
	if s.lo != nil {
		s.pos = sort.Search(len(s.ix.Entries), func(i int) bool {
			c := s.ix.Cmp(s.ix.Entries[i].Key, s.lo.Val)
			if s.lo.Inclusive {
				return c >= 0
			}
			return c > 0
		})
	}
}
func (s *indexScan) Next() (Row, bool) {
	if s.done || s.pos >= len(s.ix.Entries) {
		return nil, false
	}
	e := s.ix.Entries[s.pos]
	if s.hi != nil {
		c := s.ix.Cmp(e.Key, s.hi.Val)
		if c > 0 || (c == 0 && !s.hi.Inclusive) {
			s.done = true
			return nil, false
		}
	}
	s.pos++
	s.st.RowsRead++
	return s.t.Rows[e.RowID], true
}

func main() {
	t := &Table{Name: "t", Cols: []string{"k", "v"}, Rows: []Row{
		{int64(30), "c"}, {int64(10), "a"}, {nil, "n"}, {int64(20), "b"}, {int64(20), "b2"}, {int64(40), "d"},
	}}
	idx := BuildIndex(t, 0)
	var st Stats
	lo, hi := &Bound{int64(20), true}, &Bound{int64(40), false}
	fmt.Println(Collect(NewIndexScan(t, idx, lo, hi, &st)), "rows read:", st.RowsRead)
}
