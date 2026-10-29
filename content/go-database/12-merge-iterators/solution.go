package main

import "fmt"

type Entry struct {
	Key    string
	Value  []byte
	Delete bool
}

type Iterator interface {
	Next() (Entry, bool)
}

type sliceIter struct {
	entries []Entry
	pos     int
}

func NewSliceIter(entries []Entry) Iterator { return &sliceIter{entries: entries} }

func (s *sliceIter) Next() (Entry, bool) {
	if s.pos >= len(s.entries) {
		return Entry{}, false
	}
	e := s.entries[s.pos]
	s.pos++
	return e, true
}

func Collect(it Iterator) []Entry {
	out := []Entry{}
	for {
		e, ok := it.Next()
		if !ok {
			return out
		}
		out = append(out, e)
	}
}

type mergeIter struct {
	src   []Iterator
	heads []Entry
	have  []bool
	dead  []bool
}

func Merge(sources ...Iterator) Iterator {
	n := len(sources)
	return &mergeIter{src: sources, heads: make([]Entry, n), have: make([]bool, n), dead: make([]bool, n)}
}

func (m *mergeIter) Next() (Entry, bool) {
	for i := range m.src {
		if !m.have[i] && !m.dead[i] {
			if e, ok := m.src[i].Next(); ok {
				m.heads[i], m.have[i] = e, true
			} else {
				m.dead[i] = true
			}
		}
	}
	best := -1
	for i := range m.src {
		if m.have[i] && (best < 0 || m.heads[i].Key < m.heads[best].Key) {
			best = i
		}
	}
	if best < 0 {
		return Entry{}, false
	}
	win := m.heads[best]
	for i := range m.src {
		if m.have[i] && m.heads[i].Key == win.Key {
			m.have[i] = false
		}
	}
	return win, true
}

type liveIter struct{ it Iterator }

func Live(it Iterator) Iterator { return &liveIter{it} }

func (l *liveIter) Next() (Entry, bool) {
	for {
		e, ok := l.it.Next()
		if !ok || !e.Delete {
			return e, ok
		}
	}
}

type rangeIter struct {
	it         Iterator
	start, end string
	done       bool
}

func Range(it Iterator, start, end string) Iterator {
	return &rangeIter{it: it, start: start, end: end}
}

func (r *rangeIter) Next() (Entry, bool) {
	for !r.done {
		e, ok := r.it.Next()
		if !ok || (r.end != "" && e.Key >= r.end) {
			r.done = true
			break
		}
		if e.Key >= r.start {
			return e, true
		}
	}
	return Entry{}, false
}

// Try it with Run.
func main() {
	newer := NewSliceIter([]Entry{{Key: "b", Value: []byte("B2")}, {Key: "c", Delete: true}})
	older := NewSliceIter([]Entry{{Key: "a", Value: []byte("A1")}, {Key: "b", Value: []byte("B1")}, {Key: "c", Value: []byte("C1")}, {Key: "d", Value: []byte("D1")}})
	for _, e := range Collect(Range(Live(Merge(newer, older)), "a", "d")) {
		fmt.Printf("%s=%s\n", e.Key, e.Value)
	}
}
