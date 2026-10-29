package main

import (
	"bytes"
	"encoding/binary"
	"errors"
	"fmt"
	"io"
	"sort"
)

type Entry struct {
	Key    string
	Value  []byte
	Delete bool
}

const (
	footerSize = 24
	magic      = 0x424D535354303031
)

func WriteTable(w io.Writer, entries []Entry) error {
	for i := 1; i < len(entries); i++ {
		if entries[i-1].Key >= entries[i].Key {
			return fmt.Errorf("keys must be strictly ascending: %q then %q", entries[i-1].Key, entries[i].Key)
		}
	}
	var data, index []byte
	for _, e := range entries {
		start := len(data)
		val := e.Value
		var flag byte
		if e.Delete {
			flag, val = 1, nil
		}
		data = append(data, flag)
		data = binary.BigEndian.AppendUint32(data, uint32(len(e.Key)))
		data = binary.BigEndian.AppendUint32(data, uint32(len(val)))
		data = append(data, e.Key...)
		data = append(data, val...)

		index = binary.BigEndian.AppendUint32(index, uint32(len(e.Key)))
		index = append(index, e.Key...)
		index = binary.BigEndian.AppendUint64(index, uint64(start))
		index = binary.BigEndian.AppendUint32(index, uint32(len(data)-start))
	}
	out := append(data, index...)
	out = binary.BigEndian.AppendUint64(out, uint64(len(data)))
	out = binary.BigEndian.AppendUint64(out, uint64(len(entries)))
	out = binary.BigEndian.AppendUint64(out, magic)
	n, err := w.Write(out)
	if err == nil && n != len(out) {
		return io.ErrShortWrite
	}
	return err
}

type indexEntry struct {
	key    string
	off    int64
	length uint32
}

type Table struct {
	r     io.ReaderAt
	index []indexEntry
}

var errBad = errors.New("sstable: damaged or not a table")

func readFull(r io.ReaderAt, buf []byte, off int64) error {
	if len(buf) == 0 {
		return nil
	}
	n, err := r.ReadAt(buf, off)
	if n < len(buf) {
		if err == nil || err == io.EOF {
			err = io.ErrUnexpectedEOF
		}
		return err
	}
	return nil
}

func OpenTable(r io.ReaderAt, size int64) (*Table, error) {
	if size < footerSize {
		return nil, errBad
	}
	var f [footerSize]byte
	if err := readFull(r, f[:], size-footerSize); err != nil {
		return nil, err
	}
	indexOff := binary.BigEndian.Uint64(f[0:8])
	count := binary.BigEndian.Uint64(f[8:16])
	if binary.BigEndian.Uint64(f[16:24]) != magic {
		return nil, errBad
	}
	if indexOff > uint64(size-footerSize) {
		return nil, errBad
	}
	buf := make([]byte, uint64(size-footerSize)-indexOff)
	if err := readFull(r, buf, int64(indexOff)); err != nil {
		return nil, err
	}
	t := &Table{r: r}
	var nextOff uint64
	for i := uint64(0); i < count; i++ {
		if len(buf) < 4 {
			return nil, errBad
		}
		kl := uint64(binary.BigEndian.Uint32(buf))
		if uint64(len(buf)) < 4+kl+12 {
			return nil, errBad
		}
		key := string(buf[4 : 4+kl])
		off := binary.BigEndian.Uint64(buf[4+kl:])
		length := binary.BigEndian.Uint32(buf[12+kl:])
		if off != nextOff || off > indexOff || uint64(length) > indexOff-off || uint64(length) < 9+kl {
			return nil, errBad
		}
		if len(t.index) > 0 && t.index[len(t.index)-1].key >= key {
			return nil, errBad
		}
		nextOff = off + uint64(length)
		t.index = append(t.index, indexEntry{key, int64(off), length})
		buf = buf[16+kl:]
	}
	if len(buf) != 0 || nextOff != indexOff {
		return nil, errBad
	}
	return t, nil
}

func (t *Table) Len() int { return len(t.index) }

func (t *Table) Get(key string) (Entry, bool, error) {
	i := sort.Search(len(t.index), func(i int) bool { return t.index[i].key >= key })
	if i == len(t.index) || t.index[i].key != key {
		return Entry{}, false, nil
	}
	ie := t.index[i]
	buf := make([]byte, ie.length)
	if err := readFull(t.r, buf, ie.off); err != nil {
		return Entry{}, false, err
	}
	kl := uint64(binary.BigEndian.Uint32(buf[1:5]))
	vl := uint64(binary.BigEndian.Uint32(buf[5:9]))
	if buf[0] > 1 || (buf[0] == 1 && vl != 0) || 9+kl+vl != uint64(len(buf)) || string(buf[9:9+kl]) != key {
		return Entry{}, false, errBad
	}
	e := Entry{Key: key, Delete: buf[0] == 1}
	e.Value = append([]byte(nil), buf[9+kl:]...)
	return e, true, nil
}

// Try it with Run.
func main() {
	var buf bytes.Buffer
	WriteTable(&buf, []Entry{
		{Key: "apple", Value: []byte("red")},
		{Key: "banana", Value: []byte("yellow")},
		{Key: "cherry", Delete: true},
	})
	t, err := OpenTable(bytes.NewReader(buf.Bytes()), int64(buf.Len()))
	if err != nil {
		fmt.Println(err)
		return
	}
	e, ok, _ := t.Get("banana")
	fmt.Println(t.Len(), string(e.Value), ok)
}
