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
	v, ok := db.data[key]
	return v, ok
}

func (db *DB) Log() []byte { return append([]byte(nil), db.log...) }

func (db *DB) apply(ops []txOp) {
	for _, o := range ops {
		if o.del {
			delete(db.data, o.key)
		} else {
			db.data[o.key] = o.value
		}
	}
}

type Tx struct {
	db   *DB
	ops  []txOp
	done bool
}

func (db *DB) Begin() *Tx { return &Tx{db: db} }

func (tx *Tx) Put(key, value string) { tx.ops = append(tx.ops, txOp{false, key, value}) }
func (tx *Tx) Delete(key string)     { tx.ops = append(tx.ops, txOp{true, key, ""}) }

func (tx *Tx) Get(key string) (string, bool) {
	for i := len(tx.ops) - 1; i >= 0; i-- {
		if tx.ops[i].key == key {
			if tx.ops[i].del {
				return "", false
			}
			return tx.ops[i].value, true
		}
	}
	return tx.db.Get(key)
}

func encodeBatch(ops []txOp) []byte {
	p := binary.BigEndian.AppendUint32(nil, uint32(len(ops)))
	for _, o := range ops {
		op := byte(1)
		if o.del {
			op = 2
		}
		p = append(p, op)
		p = binary.BigEndian.AppendUint32(p, uint32(len(o.key)))
		p = binary.BigEndian.AppendUint32(p, uint32(len(o.value)))
		p = append(p, o.key...)
		p = append(p, o.value...)
	}
	return p
}

func decodeBatch(p []byte) ([]txOp, bool) {
	if len(p) < 4 {
		return nil, false
	}
	count := binary.BigEndian.Uint32(p)
	p = p[4:]
	var ops []txOp
	for i := uint32(0); i < count; i++ {
		if len(p) < 9 || (p[0] != 1 && p[0] != 2) {
			return nil, false
		}
		kl := uint64(binary.BigEndian.Uint32(p[1:5]))
		vl := uint64(binary.BigEndian.Uint32(p[5:9]))
		if uint64(len(p)-9) < kl+vl {
			return nil, false
		}
		ops = append(ops, txOp{p[0] == 2, string(p[9 : 9+kl]), string(p[9+kl : 9+kl+vl])})
		p = p[9+kl+vl:]
	}
	return ops, len(p) == 0
}

func (tx *Tx) Commit() error {
	if tx.done {
		return ErrTxDone
	}
	tx.done = true
	if len(tx.ops) == 0 {
		return nil
	}
	tx.db.log = append(tx.db.log, frame(encodeBatch(tx.ops))...)
	tx.db.apply(tx.ops)
	return nil
}

func (tx *Tx) Rollback() {
	tx.done = true
	tx.ops = nil
}

func Recover(log []byte) *DB {
	db := NewDB()
	pos := 0
	for pos < len(log) {
		p, n, ok := unframe(log[pos:])
		if !ok {
			break
		}
		ops, ok := decodeBatch(p)
		if !ok {
			break
		}
		db.apply(ops)
		pos += n
	}
	db.log = append([]byte(nil), log[:pos]...)
	return db
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
