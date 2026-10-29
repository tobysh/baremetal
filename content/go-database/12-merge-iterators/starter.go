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

func Merge(sources ...Iterator) Iterator {
	return NewSliceIter(nil)
}

func Live(it Iterator) Iterator {
	return it
}

func Range(it Iterator, start, end string) Iterator {
	return it
}

// Try it with Run.
func main() {
	newer := NewSliceIter([]Entry{{Key: "b", Value: []byte("B2")}, {Key: "c", Delete: true}})
	older := NewSliceIter([]Entry{{Key: "a", Value: []byte("A1")}, {Key: "b", Value: []byte("B1")}, {Key: "c", Value: []byte("C1")}, {Key: "d", Value: []byte("D1")}})
	for _, e := range Collect(Range(Live(Merge(newer, older)), "a", "d")) {
		fmt.Printf("%s=%s\n", e.Key, e.Value)
	}
}
