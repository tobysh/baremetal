package main

import (
	"errors"
	"fmt"
	"reflect"
	"testing"
)

func snapshotOf(db *DB, keys []string) map[string]string {
	m := map[string]string{}
	for _, k := range keys {
		if v, ok := db.Get(k); ok {
			m[k] = v
		}
	}
	return m
}

var txKeys = []string{"a", "b", "c", "d", "e"}

func TestCommit_applies_and_rollback_discards(t *testing.T) {
	db := NewDB()
	tx := db.Begin()
	tx.Put("a", "1")
	tx.Put("b", "2")
	if _, ok := db.Get("a"); ok {
		t.Errorf("the database saw a write before Commit")
	}
	if err := tx.Commit(); err != nil {
		t.Fatalf("Commit: %v", err)
	}
	if v, _ := db.Get("a"); v != "1" {
		t.Errorf("a = %q after Commit; want 1", v)
	}
	tx2 := db.Begin()
	tx2.Put("a", "changed")
	tx2.Delete("b")
	before := len(db.Log())
	tx2.Rollback()
	if v, _ := db.Get("a"); v != "1" {
		t.Errorf("a = %q after Rollback; want 1", v)
	}
	if _, ok := db.Get("b"); !ok {
		t.Errorf("b was deleted by a rolled-back transaction")
	}
	if len(db.Log()) != before {
		t.Errorf("Rollback wrote to the log")
	}
}

func TestTx_reads_its_own_writes(t *testing.T) {
	db := NewDB()
	setup := db.Begin()
	setup.Put("x", "base")
	setup.Put("y", "base")
	setup.Commit()

	tx := db.Begin()
	if v, ok := tx.Get("x"); !ok || v != "base" {
		t.Errorf("tx.Get(x) = %q, %v; want base from the database", v, ok)
	}
	tx.Put("x", "one")
	tx.Put("x", "two")
	tx.Delete("y")
	tx.Put("z", "new")
	if v, _ := tx.Get("x"); v != "two" {
		t.Errorf("tx.Get(x) = %q; want the latest pending write, two", v)
	}
	if _, ok := tx.Get("y"); ok {
		t.Errorf("tx.Get(y) found a key the transaction deleted")
	}
	if v, ok := tx.Get("z"); !ok || v != "new" {
		t.Errorf("tx.Get(z) = %q, %v", v, ok)
	}
	tx.Put("y", "again")
	if v, ok := tx.Get("y"); !ok || v != "again" {
		t.Errorf("tx.Get(y) after delete then put = %q, %v", v, ok)
	}
	if v, _ := db.Get("x"); v != "base" {
		t.Errorf("db.Get(x) = %q before Commit; want base", v)
	}
}

func TestOperations_apply_in_order(t *testing.T) {
	db := NewDB()
	tx := db.Begin()
	tx.Put("k", "1")
	tx.Delete("k")
	tx.Put("k", "3")
	tx.Put("gone", "x")
	tx.Delete("gone")
	tx.Commit()
	if v, _ := db.Get("k"); v != "3" {
		t.Errorf("k = %q; want 3", v)
	}
	if _, ok := db.Get("gone"); ok {
		t.Errorf("gone should be deleted")
	}
}

func TestFinished_transactions(t *testing.T) {
	db := NewDB()
	tx := db.Begin()
	tx.Put("a", "1")
	if err := tx.Commit(); err != nil {
		t.Fatal(err)
	}
	if err := tx.Commit(); !errors.Is(err, ErrTxDone) {
		t.Errorf("second Commit = %v; want ErrTxDone", err)
	}
	tx.Rollback()
	if v, _ := db.Get("a"); v != "1" {
		t.Errorf("Rollback after Commit undid the commit")
	}
	tx = db.Begin()
	tx.Put("b", "2")
	tx.Rollback()
	if err := tx.Commit(); !errors.Is(err, ErrTxDone) {
		t.Errorf("Commit after Rollback = %v; want ErrTxDone", err)
	}
	if _, ok := db.Get("b"); ok {
		t.Errorf("b was written by a rolled-back transaction")
	}
}

func TestCommit_writes_exactly_one_frame(t *testing.T) {
	db := NewDB()
	tx := db.Begin()
	for i := 0; i < 20; i++ {
		tx.Put(fmt.Sprintf("k%d", i), "v")
	}
	tx.Commit()
	log := db.Log()
	_, n, ok := unframe(log)
	if !ok || n != len(log) {
		t.Errorf("the log of one transaction should be one valid frame; first frame is %d of %d bytes (ok=%v)", n, len(log), ok)
	}
	empty := db.Begin()
	empty.Commit()
	if len(db.Log()) != len(log) {
		t.Errorf("committing an empty transaction wrote %d bytes", len(db.Log())-len(log))
	}
}

func TestRecover_is_atomic_at_every_cut(t *testing.T) {
	db := NewDB()
	var states []map[string]string
	var ends []int
	states = append(states, map[string]string{})
	scripts := [][]txOp{
		{{false, "a", "1"}, {false, "b", "2"}},
		{{false, "a", "10"}, {true, "b", ""}, {false, "c", "3"}},
		{{false, "d", "four"}},
		{{true, "a", ""}, {true, "c", ""}, {false, "e", "five"}, {false, "b", "back"}},
	}
	for _, ops := range scripts {
		tx := db.Begin()
		for _, o := range ops {
			if o.del {
				tx.Delete(o.key)
			} else {
				tx.Put(o.key, o.value)
			}
		}
		if err := tx.Commit(); err != nil {
			t.Fatal(err)
		}
		states = append(states, snapshotOf(db, txKeys))
		ends = append(ends, len(db.Log()))
	}
	log := db.Log()
	for cut := 0; cut <= len(log); cut++ {
		committed, good := 0, 0
		for i, e := range ends {
			if e <= cut {
				committed, good = i+1, e
			}
		}
		rec := Recover(log[:cut])
		if got := snapshotOf(rec, txKeys); !reflect.DeepEqual(got, states[committed]) {
			t.Fatalf("log cut at %d: recovered %v; want the state after %d whole transactions: %v", cut, got, committed, states[committed])
		}
		if len(rec.Log()) != good {
			t.Fatalf("log cut at %d: recovered log is %d bytes; want %d", cut, len(rec.Log()), good)
		}
	}
}

func TestRecover_then_commit_again(t *testing.T) {
	db := NewDB()
	tx := db.Begin()
	tx.Put("a", "1")
	tx.Commit()
	good := len(db.Log())
	tx = db.Begin()
	tx.Put("b", "2")
	tx.Commit()
	torn := db.Log()
	torn = torn[:good+5]

	rec := Recover(torn)
	tx = rec.Begin()
	tx.Put("c", "3")
	tx.Commit()
	again := Recover(rec.Log())
	want := map[string]string{"a": "1", "c": "3"}
	if got := snapshotOf(again, txKeys); !reflect.DeepEqual(got, want) {
		t.Errorf("after recover, commit and recover again: %v; want %v", got, want)
	}
}

func TestRecover_stops_at_corruption(t *testing.T) {
	db := NewDB()
	var ends []int
	for i := 0; i < 4; i++ {
		tx := db.Begin()
		tx.Put(fmt.Sprintf("k%d", i), "v")
		tx.Put("shared", fmt.Sprint(i))
		tx.Commit()
		ends = append(ends, len(db.Log()))
	}
	bad := db.Log()
	bad[ends[1]+12] ^= 0x20
	rec := Recover(bad)
	if v, _ := rec.Get("shared"); v != "1" {
		t.Errorf("shared = %q; want 1 (state after the last intact transaction)", v)
	}
	if _, ok := rec.Get("k2"); ok {
		t.Errorf("a transaction after the damaged frame was applied")
	}
	if len(rec.Log()) != ends[1] {
		t.Errorf("recovered log is %d bytes; want %d", len(rec.Log()), ends[1])
	}
}
