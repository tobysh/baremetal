package main

import (
	"bytes"
	"fmt"
	"io"
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
	return nil
}

type Table struct {
	r io.ReaderAt
}

func OpenTable(r io.ReaderAt, size int64) (*Table, error) {
	return &Table{r: r}, nil
}

func (t *Table) Len() int { return 0 }

func (t *Table) Get(key string) (Entry, bool, error) {
	return Entry{}, false, nil
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
