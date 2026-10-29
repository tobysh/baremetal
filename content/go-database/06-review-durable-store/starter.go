package main

import (
	"fmt"
)

type DB struct {
	m   map[string]string
	log []byte
}

func Open(log []byte) *DB {
	return &DB{m: map[string]string{}}
}

func (db *DB) Put(key, value string) {}
func (db *DB) Delete(key string)     {}

func (db *DB) Get(key string) (string, bool) {
	return "", false
}

func (db *DB) Log() []byte { return nil }

func (db *DB) Compact() {}

// Try it with Run.
func main() {
	db := Open(nil)
	db.Put("a", "1")
	db.Put("a", "2")
	db.Delete("b")
	before := len(db.Log())
	db.Compact()
	fmt.Println(before, "->", len(db.Log()))
	again := Open(db.Log())
	fmt.Println(again.Get("a"))
}
