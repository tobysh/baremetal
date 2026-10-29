package main

import (
	"fmt"
	"sort"
)

type Entry struct {
	Key    string
	Value  []byte
	Delete bool
}

// tableReads counts calls to table.get so tests can see how far a lookup went.
var tableReads int

// table is an immutable sorted run of entries.
type table struct {
	entries []Entry
}

func newTable(sorted []Entry) *table { return &table{entries: sorted} }

func (t *table) get(key string) (Entry, bool) {
	tableReads++
	i := sort.Search(len(t.entries), func(i int) bool { return t.entries[i].Key >= key })
	if i < len(t.entries) && t.entries[i].Key == key {
		return t.entries[i], true
	}
	return Entry{}, false
}

type DB struct {
	limit  int
	mem    map[string]Entry
	tables []*table // newest first
}

func NewDB(memLimit int) *DB {
	return &DB{limit: memLimit, mem: map[string]Entry{}}
}

func (db *DB) Put(key string, value []byte) {
}

func (db *DB) Delete(key string) {
}

func (db *DB) Flush() {
}

func (db *DB) Get(key string) ([]byte, bool) {
	return nil, false
}

func (db *DB) TableCount() int { return 0 }
func (db *DB) MemLen() int     { return 0 }

// Try it with Run.
func main() {
	db := NewDB(3)
	for i, k := range []string{"a", "b", "c", "d"} {
		db.Put(k, []byte{byte('0' + i)})
	}
	db.Delete("a")
	v, ok := db.Get("b")
	_, okA := db.Get("a")
	fmt.Println(string(v), ok, okA, db.TableCount(), db.MemLen())
}
