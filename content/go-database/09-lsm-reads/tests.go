package main

import (
	"fmt"
	"math/rand"
	"testing"
)

func mustGet(t *testing.T, db *DB, key, want string) {
	t.Helper()
	v, ok := db.Get(key)
	if !ok || string(v) != want {
		t.Errorf("Get(%q) = %q, %v; want %q", key, v, ok, want)
	}
}

func mustMiss(t *testing.T, db *DB, key string) {
	t.Helper()
	if v, ok := db.Get(key); ok {
		t.Errorf("Get(%q) = %q, true; want not found", key, v)
	}
}

func TestMemtable_only(t *testing.T) {
	db := NewDB(100)
	db.Put("a", []byte("1"))
	db.Put("b", []byte("2"))
	db.Put("a", []byte("3"))
	mustGet(t, db, "a", "3")
	mustGet(t, db, "b", "2")
	mustMiss(t, db, "c")
	if db.TableCount() != 0 || db.MemLen() != 2 {
		t.Errorf("tables %d, mem %d; want 0 and 2", db.TableCount(), db.MemLen())
	}
}

func TestAutomatic_flush_at_the_limit(t *testing.T) {
	db := NewDB(3)
	db.Put("a", []byte("1"))
	db.Put("b", []byte("2"))
	if db.TableCount() != 0 {
		t.Fatalf("flushed too early: %d tables", db.TableCount())
	}
	db.Put("c", []byte("3"))
	if db.TableCount() != 1 || db.MemLen() != 0 {
		t.Errorf("at the limit: tables %d, mem %d; want 1 and 0", db.TableCount(), db.MemLen())
	}
	db.Put("a", []byte("x"))
	db.Put("a", []byte("y"))
	if db.MemLen() != 1 {
		t.Errorf("overwriting one key counted as %d entries; want 1", db.MemLen())
	}
	mustGet(t, db, "a", "y")
	mustGet(t, db, "b", "2")
}

func TestFlush_explicit_and_empty(t *testing.T) {
	db := NewDB(100)
	db.Flush()
	if db.TableCount() != 0 {
		t.Errorf("flushing an empty memtable made a table")
	}
	db.Put("z", []byte("last"))
	db.Put("m", []byte("mid"))
	db.Flush()
	if db.TableCount() != 1 || db.MemLen() != 0 {
		t.Errorf("tables %d, mem %d; want 1 and 0", db.TableCount(), db.MemLen())
	}
	mustGet(t, db, "z", "last")
	mustGet(t, db, "m", "mid")
}

func TestNewer_tables_shadow_older_ones(t *testing.T) {
	db := NewDB(100)
	db.Put("k", []byte("old"))
	db.Flush()
	db.Put("k", []byte("mid"))
	db.Flush()
	mustGet(t, db, "k", "mid")
	db.Put("k", []byte("new"))
	mustGet(t, db, "k", "new")
	db.Flush()
	mustGet(t, db, "k", "new")
	if db.TableCount() != 3 {
		t.Errorf("TableCount = %d; want 3", db.TableCount())
	}
}

func TestDelete_hides_older_values(t *testing.T) {
	db := NewDB(100)
	db.Put("k", []byte("v"))
	db.Put("other", []byte("o"))
	db.Flush()
	db.Delete("k")
	mustMiss(t, db, "k")
	db.Flush()
	mustMiss(t, db, "k")
	mustGet(t, db, "other", "o")
	db.Put("k", []byte("back"))
	mustGet(t, db, "k", "back")
	db.Flush()
	mustGet(t, db, "k", "back")
}

func TestGet_stops_at_the_first_answer(t *testing.T) {
	db := NewDB(100)
	db.Put("old", []byte("1"))
	db.Flush()
	db.Put("mid", []byte("2"))
	db.Flush()
	db.Put("new", []byte("3"))
	db.Delete("old")
	db.Flush()
	db.Put("fresh", []byte("4"))

	reads := func(key string) int {
		tableReads = 0
		db.Get(key)
		return tableReads
	}
	if n := reads("fresh"); n != 0 {
		t.Errorf("a key in the memtable cost %d table reads; want 0", n)
	}
	if n := reads("new"); n != 1 {
		t.Errorf("a key in the newest table cost %d table reads; want 1", n)
	}
	if n := reads("old"); n != 1 {
		t.Errorf("a deleted key cost %d table reads; the tombstone in the newest table is the answer, so want 1", n)
	}
	if n := reads("mid"); n != 2 {
		t.Errorf("a key in the middle table cost %d table reads; want 2", n)
	}
	if n := reads("missing"); n != 3 {
		t.Errorf("a missing key cost %d table reads; want 3 (one per table)", n)
	}
}

func TestPut_copies_the_value_and_Get_does_not_alias(t *testing.T) {
	db := NewDB(2)
	buf := []byte("hello")
	db.Put("k", buf)
	buf[0] = 'J'
	mustGet(t, db, "k", "hello")
	db.Put("other", []byte("x"))
	v, _ := db.Get("k")
	v[0] = 'Z'
	mustGet(t, db, "k", "hello")
}

func TestRandom_workload_matches_a_map(t *testing.T) {
	for _, limit := range []int{1, 4, 17} {
		rng := rand.New(rand.NewSource(int64(limit)))
		db := NewDB(limit)
		model := map[string]string{}
		for step := 0; step < 3000; step++ {
			k := fmt.Sprintf("k%02d", rng.Intn(60))
			switch rng.Intn(4) {
			case 0:
				db.Delete(k)
				delete(model, k)
			case 1:
				db.Flush()
			default:
				v := fmt.Sprint(step)
				db.Put(k, []byte(v))
				model[k] = v
			}
			if step%7 == 0 {
				probe := fmt.Sprintf("k%02d", rng.Intn(60))
				v, ok := db.Get(probe)
				mv, mok := model[probe]
				if ok != mok || string(v) != mv {
					t.Fatalf("limit %d, step %d: Get(%s) = %q, %v; want %q, %v", limit, step, probe, v, ok, mv, mok)
				}
			}
		}
	}
}
