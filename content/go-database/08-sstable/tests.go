package main

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"io"
	"testing"
)

// countingReaderAt counts ReadAt calls and, like some real files, reports io.EOF
// together with the data when a read ends exactly at the end.
type countingReaderAt struct {
	data  []byte
	reads int
}

func (c *countingReaderAt) ReadAt(p []byte, off int64) (int, error) {
	c.reads++
	if off < 0 || off > int64(len(c.data)) {
		return 0, io.EOF
	}
	n := copy(p, c.data[off:])
	if off+int64(n) >= int64(len(c.data)) {
		return n, io.EOF
	}
	return n, nil
}

func sampleEntries(n int) []Entry {
	var es []Entry
	for i := 0; i < n; i++ {
		e := Entry{Key: fmt.Sprintf("key-%04d", i), Value: []byte(fmt.Sprintf("value %d %s", i, bytes.Repeat([]byte("."), i%9)))}
		if i%7 == 3 {
			e = Entry{Key: e.Key, Delete: true}
		}
		es = append(es, e)
	}
	return es
}

func writeTable(t *testing.T, es []Entry) []byte {
	t.Helper()
	var buf bytes.Buffer
	if err := WriteTable(&buf, es); err != nil {
		t.Fatalf("WriteTable: %v", err)
	}
	return buf.Bytes()
}

func TestWriteTable_footer(t *testing.T) {
	data := writeTable(t, sampleEntries(5))
	if len(data) < 24 {
		t.Fatalf("table is only %d bytes", len(data))
	}
	f := data[len(data)-24:]
	if got := binary.BigEndian.Uint64(f[16:]); got != 0x424D535354303031 {
		t.Errorf("magic = %#x; want 0x424D535354303031", got)
	}
	if got := binary.BigEndian.Uint64(f[8:]); got != 5 {
		t.Errorf("footer count = %d; want 5", got)
	}
	if off := binary.BigEndian.Uint64(f[0:]); off == 0 || off > uint64(len(data)-24) {
		t.Errorf("footer index offset = %d is outside the table (%d bytes)", off, len(data))
	}
	// First entry: flag, keyLen, valLen, key, value.
	if data[0] != 0 || binary.BigEndian.Uint32(data[1:5]) != 8 || string(data[9:17]) != "key-0000" {
		t.Errorf("first entry is not laid out as flag|keyLen|valLen|key|value: % x", data[:20])
	}
}

func TestOpenTable_and_Get_every_key(t *testing.T) {
	es := sampleEntries(200)
	data := writeTable(t, es)
	tbl, err := OpenTable(bytes.NewReader(data), int64(len(data)))
	if err != nil {
		t.Fatalf("OpenTable: %v", err)
	}
	if tbl.Len() != 200 {
		t.Errorf("Len = %d; want 200", tbl.Len())
	}
	for _, want := range es {
		got, ok, err := tbl.Get(want.Key)
		if err != nil || !ok {
			t.Fatalf("Get(%s) = ok %v, err %v", want.Key, ok, err)
		}
		if got.Key != want.Key || got.Delete != want.Delete || !bytes.Equal(got.Value, want.Value) {
			t.Fatalf("Get(%s) = %+v; want %+v", want.Key, got, want)
		}
	}
}

func TestGet_missing_keys(t *testing.T) {
	data := writeTable(t, sampleEntries(10))
	tbl, _ := OpenTable(bytes.NewReader(data), int64(len(data)))
	for _, k := range []string{"", "a", "key-0003x", "key-9999", "zzz"} {
		if _, ok, err := tbl.Get(k); ok || err != nil {
			t.Errorf("Get(%q) = ok %v, err %v; want not found", k, ok, err)
		}
	}
}

func TestGet_costs_one_read_and_misses_cost_none(t *testing.T) {
	data := writeTable(t, sampleEntries(100))
	src := &countingReaderAt{data: data}
	tbl, err := OpenTable(src, int64(len(data)))
	if err != nil {
		t.Fatalf("OpenTable with a ReaderAt that returns io.EOF at the end: %v", err)
	}
	if src.reads > 2 {
		t.Errorf("OpenTable used %d reads; want 2 (footer and index)", src.reads)
	}
	for _, k := range []string{"key-0000", "key-0050", "key-0099", "key-0001"} {
		before := src.reads
		if _, ok, err := tbl.Get(k); !ok || err != nil {
			t.Fatalf("Get(%s) = ok %v, err %v", k, ok, err)
		}
		if used := src.reads - before; used != 1 {
			t.Errorf("Get(%s) used %d reads; want exactly 1", k, used)
		}
	}
	before := src.reads
	tbl.Get("not-there")
	tbl.Get("key-0050x")
	if src.reads != before {
		t.Errorf("looking up missing keys used %d reads; the in-memory index should answer those", src.reads-before)
	}
}

func TestTombstones_round_trip(t *testing.T) {
	es := []Entry{{Key: "a", Value: []byte("1")}, {Key: "b", Delete: true, Value: []byte("ignored")}, {Key: "c", Value: []byte{}}}
	data := writeTable(t, es)
	tbl, _ := OpenTable(bytes.NewReader(data), int64(len(data)))
	e, ok, _ := tbl.Get("b")
	if !ok || !e.Delete || len(e.Value) != 0 {
		t.Errorf("tombstone = %+v, found %v; want Delete with no value", e, ok)
	}
	e, ok, _ = tbl.Get("c")
	if !ok || e.Delete || len(e.Value) != 0 {
		t.Errorf("empty value = %+v, found %v", e, ok)
	}
}

func TestEmpty_table(t *testing.T) {
	data := writeTable(t, nil)
	if len(data) != 24 {
		t.Errorf("an empty table is %d bytes; want just the 24-byte footer", len(data))
	}
	tbl, err := OpenTable(bytes.NewReader(data), int64(len(data)))
	if err != nil || tbl.Len() != 0 {
		t.Fatalf("OpenTable on empty table: %v", err)
	}
	if _, ok, _ := tbl.Get("x"); ok {
		t.Errorf("found a key in an empty table")
	}
}

func TestWriteTable_rejects_unsorted_and_duplicate_keys(t *testing.T) {
	for name, es := range map[string][]Entry{
		"unsorted":  {{Key: "b"}, {Key: "a"}},
		"duplicate": {{Key: "a"}, {Key: "a"}},
	} {
		var buf bytes.Buffer
		if err := WriteTable(&buf, es); err == nil {
			t.Errorf("%s keys were accepted", name)
		}
		if buf.Len() != 0 {
			t.Errorf("%s keys: %d bytes were written before the error", name, buf.Len())
		}
	}
}

func TestOpenTable_rejects_damaged_files(t *testing.T) {
	data := writeTable(t, sampleEntries(20))
	try := func(name string, b []byte) {
		defer func() {
			if r := recover(); r != nil {
				t.Errorf("%s: OpenTable panicked: %v", name, r)
			}
		}()
		if _, err := OpenTable(bytes.NewReader(b), int64(len(b))); err == nil {
			t.Errorf("%s: OpenTable returned no error", name)
		}
	}
	try("empty file", nil)
	try("shorter than a footer", data[:10])
	try("truncated by one byte", data[:len(data)-1])
	try("truncated in the middle", data[:len(data)/2])
	bad := append([]byte{}, data...)
	bad[len(bad)-1] ^= 0xFF
	try("wrong magic", bad)
	bad = append([]byte{}, data...)
	binary.BigEndian.PutUint64(bad[len(bad)-24:], uint64(len(bad)+100))
	try("index offset past the end", bad)
	bad = append([]byte{}, data...)
	binary.BigEndian.PutUint64(bad[len(bad)-16:], 1<<40)
	try("absurd entry count", bad)
	bad = append([]byte{}, data...)
	binary.BigEndian.PutUint64(bad[len(bad)-16:], 21)
	try("count larger than the index", bad)
}
