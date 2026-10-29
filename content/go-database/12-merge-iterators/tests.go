package main

import (
	"fmt"
	"reflect"
	"testing"
)

func pe(k, v string) Entry { return Entry{Key: k, Value: []byte(v)} }
func te(k string) Entry    { return Entry{Key: k, Delete: true} }

func show(es []Entry) []string {
	out := []string{}
	for _, e := range es {
		if e.Delete {
			out = append(out, e.Key+"=<del>")
		} else {
			out = append(out, e.Key+"="+string(e.Value))
		}
	}
	return out
}

// counting wraps an iterator and counts the entries pulled from it.
type counting struct {
	it    Iterator
	pulls int
}

func (c *counting) Next() (Entry, bool) {
	c.pulls++
	return c.it.Next()
}

func TestMerge_interleaves_in_key_order(t *testing.T) {
	a := NewSliceIter([]Entry{pe("a", "1"), pe("d", "4"), pe("g", "7")})
	b := NewSliceIter([]Entry{pe("b", "2"), pe("e", "5")})
	c := NewSliceIter([]Entry{pe("c", "3"), pe("f", "6"), pe("h", "8")})
	got := show(Collect(Merge(a, b, c)))
	want := []string{"a=1", "b=2", "c=3", "d=4", "e=5", "f=6", "g=7", "h=8"}
	if !reflect.DeepEqual(got, want) {
		t.Errorf("merge = %v; want %v", got, want)
	}
}

func TestMerge_newest_source_wins(t *testing.T) {
	newest := NewSliceIter([]Entry{pe("b", "new"), te("c")})
	middle := NewSliceIter([]Entry{pe("b", "mid"), pe("c", "mid"), pe("e", "mid")})
	oldest := NewSliceIter([]Entry{pe("a", "old"), pe("b", "old"), pe("c", "old"), pe("d", "old"), pe("e", "old")})
	got := show(Collect(Merge(newest, middle, oldest)))
	want := []string{"a=old", "b=new", "c=<del>", "d=old", "e=mid"}
	if !reflect.DeepEqual(got, want) {
		t.Errorf("merge = %v; want %v", got, want)
	}
}

func TestMerge_edge_cases(t *testing.T) {
	if got := Collect(Merge()); len(got) != 0 {
		t.Errorf("Merge() of nothing = %v", got)
	}
	if got := Collect(Merge(NewSliceIter(nil), NewSliceIter(nil))); len(got) != 0 {
		t.Errorf("merge of empty sources = %v", got)
	}
	got := show(Collect(Merge(NewSliceIter(nil), NewSliceIter([]Entry{pe("x", "1")}), NewSliceIter(nil))))
	if !reflect.DeepEqual(got, []string{"x=1"}) {
		t.Errorf("merge with empty sources around = %v", got)
	}
	it := Merge(NewSliceIter([]Entry{pe("a", "1")}))
	it.Next()
	for i := 0; i < 3; i++ {
		if _, ok := it.Next(); ok {
			t.Errorf("an exhausted iterator returned an entry")
		}
	}
}

func TestMerge_is_lazy(t *testing.T) {
	var big1, big2 []Entry
	for i := 0; i < 5000; i++ {
		big1 = append(big1, pe(fmt.Sprintf("%05d", 2*i), "a"))
		big2 = append(big2, pe(fmt.Sprintf("%05d", 2*i+1), "b"))
	}
	c1 := &counting{it: NewSliceIter(big1)}
	c2 := &counting{it: NewSliceIter(big2)}
	m := Merge(c1, c2)
	for i := 0; i < 3; i++ {
		e, ok := m.Next()
		if !ok || e.Key != fmt.Sprintf("%05d", i) {
			t.Fatalf("entry %d = %q, %v", i, e.Key, ok)
		}
	}
	if c1.pulls+c2.pulls > 8 {
		t.Errorf("taking 3 entries pulled %d from the sources; a lazy merge needs about 5", c1.pulls+c2.pulls)
	}
}

func TestLive_skips_tombstones(t *testing.T) {
	src := NewSliceIter([]Entry{te("a"), pe("b", "1"), te("c"), te("d"), pe("e", "2"), te("f")})
	got := show(Collect(Live(src)))
	if !reflect.DeepEqual(got, []string{"b=1", "e=2"}) {
		t.Errorf("Live = %v", got)
	}
	if got := Collect(Live(NewSliceIter([]Entry{te("a")}))); len(got) != 0 {
		t.Errorf("Live of only tombstones = %v", got)
	}
}

func TestLive_must_come_after_merge(t *testing.T) {
	newer := NewSliceIter([]Entry{te("k")})
	older := NewSliceIter([]Entry{pe("j", "1"), pe("k", "old"), pe("l", "2")})
	got := show(Collect(Live(Merge(newer, older))))
	if !reflect.DeepEqual(got, []string{"j=1", "l=2"}) {
		t.Errorf("Live(Merge(..)) = %v; want the deleted key k to stay hidden", got)
	}
}

func TestRange_bounds(t *testing.T) {
	var es []Entry
	for _, k := range []string{"a", "b", "c", "d", "e", "f"} {
		es = append(es, pe(k, k))
	}
	cases := []struct {
		start, end string
		want       []string
	}{
		{"b", "e", []string{"b=b", "c=c", "d=d"}},
		{"", "c", []string{"a=a", "b=b"}},
		{"d", "", []string{"d=d", "e=e", "f=f"}},
		{"", "", []string{"a=a", "b=b", "c=c", "d=d", "e=e", "f=f"}},
		{"bb", "dd", []string{"c=c", "d=d"}},
		{"g", "", []string{}},
		{"c", "c", []string{}},
		{"x", "y", []string{}},
	}
	for _, c := range cases {
		got := show(Collect(Range(NewSliceIter(es), c.start, c.end)))
		if !reflect.DeepEqual(got, c.want) {
			t.Errorf("Range(%q, %q) = %v; want %v", c.start, c.end, got, c.want)
		}
	}
}

func TestRange_stops_without_draining(t *testing.T) {
	var es []Entry
	for i := 0; i < 10000; i++ {
		es = append(es, pe(fmt.Sprintf("%05d", i), "v"))
	}
	c := &counting{it: NewSliceIter(es)}
	r := Range(c, "00010", "00015")
	got := Collect(r)
	if len(got) != 5 {
		t.Fatalf("got %d entries; want 5", len(got))
	}
	if c.pulls > 17 {
		t.Errorf("the range read %d entries from the source; it should stop at the first key past the end (about 16)", c.pulls)
	}
	before := c.pulls
	r.Next()
	r.Next()
	if c.pulls != before {
		t.Errorf("a finished range kept reading from the source")
	}
}

func TestStacked_scan_over_memtable_and_tables(t *testing.T) {
	mem := NewSliceIter([]Entry{pe("b", "mem"), te("d")})
	t2 := NewSliceIter([]Entry{pe("a", "t2"), pe("c", "t2"), pe("d", "t2"), pe("f", "t2")})
	t1 := NewSliceIter([]Entry{pe("a", "t1"), pe("b", "t1"), pe("e", "t1"), pe("g", "t1")})
	got := show(Collect(Range(Live(Merge(mem, t2, t1)), "b", "g")))
	want := []string{"b=mem", "c=t2", "e=t1", "f=t2"}
	if !reflect.DeepEqual(got, want) {
		t.Errorf("scan = %v; want %v", got, want)
	}
}
