package main

import (
	"encoding/binary"
	"errors"
	"fmt"
	"hash/crc32"
)

var ErrTxDone = errors.New("transaction already finished")

func frame(payload []byte) []byte {
	f := make([]byte, 8+len(payload))
	binary.BigEndian.PutUint32(f[4:8], uint32(len(payload)))
	copy(f[8:], payload)
	binary.BigEndian.PutUint32(f[0:4], crc32.ChecksumIEEE(f[4:]))
	return f
}

// unframe returns the payload and total size of the frame at the start of data.
// ok is false if the frame is incomplete or its checksum is wrong.
func unframe(data []byte) (payload []byte, n int, ok bool) {
	if len(data) < 8 {
		return nil, 0, false
	}
	size := uint64(binary.BigEndian.Uint32(data[4:8]))
	if uint64(len(data)-8) < size {
		return nil, 0, false
	}
	total := 8 + int(size)
	if crc32.ChecksumIEEE(data[4:total]) != binary.BigEndian.Uint32(data[0:4]) {
		return nil, 0, false
	}
	return data[8:total], total, true
}

type txOp struct {
	del        bool
	key, value string
}

type DB struct {
	data map[string]string
	log  []byte
}

func NewDB() *DB { return &DB{data: map[string]string{}} }

func (db *DB) Get(key string) (string, bool) {
	return "", false
}

func (db *DB) Log() []byte { return nil }

type Tx struct {
	db   *DB
	ops  []txOp
	done bool
}

func (db *DB) Begin() *Tx { return &Tx{db: db} }

func (tx *Tx) Put(key, value string) {}
func (tx *Tx) Delete(key string)     {}

func (tx *Tx) Get(key string) (string, bool) {
	return "", false
}

func (tx *Tx) Commit() error {
	return nil
}

func (tx *Tx) Rollback() {}

func Recover(log []byte) *DB {
	return NewDB()
}

// Try it with Run.
func main() {
	db := NewDB()
	tx := db.Begin()
	tx.Put("alice", "90")
	tx.Put("bob", "110")
	fmt.Println(tx.Get("alice"))
	fmt.Println(db.Get("alice"))
	tx.Commit()
	fmt.Println(db.Get("alice"))
	fmt.Println(len(Recover(db.Log()).data))
}
