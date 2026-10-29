package main

import (
	"encoding/binary"
	"fmt"
	"hash/crc32"
	"sort"
)

type DB struct {
	m   map[string]string
	log []byte
}

func frame(payload []byte) []byte {
	f := make([]byte, 8+len(payload))
	binary.BigEndian.PutUint32(f[4:8], uint32(len(payload)))
	copy(f[8:], payload)
	binary.BigEndian.PutUint32(f[0:4], crc32.ChecksumIEEE(f[4:]))
	return f
}

func unframe(data []byte) ([]byte, int, bool) {
	if len(data) < 8 {
		return nil, 0, false
	}
	n := uint64(binary.BigEndian.Uint32(data[4:8]))
	if uint64(len(data)-8) < n {
		return nil, 0, false
	}
	total := 8 + int(n)
	if crc32.ChecksumIEEE(data[4:total]) != binary.BigEndian.Uint32(data[0:4]) {
		return nil, 0, false
	}
	return data[8:total], total, true
}

func encodeOp(op byte, key, value string) []byte {
	p := []byte{op}
	p = binary.BigEndian.AppendUint32(p, uint32(len(key)))
	p = append(p, key...)
	return append(p, value...)
}

// apply decodes a payload and applies it; false means the payload is nonsense.
func (db *DB) apply(p []byte) bool {
	if len(p) < 5 || (p[0] != 1 && p[0] != 2) {
		return false
	}
	kl := uint64(binary.BigEndian.Uint32(p[1:5]))
	if uint64(len(p)-5) < kl {
		return false
	}
	key := string(p[5 : 5+kl])
	if p[0] == 1 {
		db.m[key] = string(p[5+kl:])
	} else {
		delete(db.m, key)
	}
	return true
}

func Open(log []byte) *DB {
	db := &DB{m: map[string]string{}}
	pos := 0
	for pos < len(log) {
		p, n, ok := unframe(log[pos:])
		if !ok || !db.apply(p) {
			break
		}
		pos += n
	}
	db.log = append([]byte(nil), log[:pos]...)
	return db
}

func (db *DB) write(payload []byte) {
	db.apply(payload)
	db.log = append(db.log, frame(payload)...)
}

func (db *DB) Put(key, value string) { db.write(encodeOp(1, key, value)) }
func (db *DB) Delete(key string)     { db.write(encodeOp(2, key, "")) }

func (db *DB) Get(key string) (string, bool) {
	v, ok := db.m[key]
	return v, ok
}

func (db *DB) Log() []byte { return append([]byte(nil), db.log...) }

func (db *DB) Compact() {
	keys := make([]string, 0, len(db.m))
	for k := range db.m {
		keys = append(keys, k)
	}
	sort.Strings(keys)
	db.log = nil
	for _, k := range keys {
		db.log = append(db.log, frame(encodeOp(1, k, db.m[k]))...)
	}
}

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
