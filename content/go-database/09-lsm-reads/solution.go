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
	db.mem[key] = Entry{Key: key, Value: append([]byte(nil), value...)}
	db.maybeFlush()
}

func (db *DB) Delete(key string) {
	db.mem[key] = Entry{Key: key, Delete: true}
	db.maybeFlush()
}

func (db *DB) maybeFlush() {
	if len(db.mem) >= db.limit {
		db.Flush()
	}
}

func (db *DB) Flush() {
	if len(db.mem) == 0 {
		return
	}
	entries := make([]Entry, 0, len(db.mem))
	for _, e := range db.mem {
		entries = append(entries, e)
	}
	sort.Slice(entries, func(i, j int) bool { return entries[i].Key < entries[j].Key })
	db.tables = append([]*table{newTable(entries)}, db.tables...)
	db.mem = map[string]Entry{}
}

func (db *DB) Get(key string) ([]byte, bool) {
	if e, ok := db.mem[key]; ok {
		return result(e)
	}
	for _, t := range db.tables {
		if e, ok := t.get(key); ok {
			return result(e)
		}
	}
	return nil, false
}

func result(e Entry) ([]byte, bool) {
	if e.Delete {
		return nil, false
	}
	return append([]byte{}, e.Value...), true
}

func (db *DB) TableCount() int { return len(db.tables) }
func (db *DB) MemLen() int     { return len(db.mem) }

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
