package main

import (
	"fmt"
	"math/rand"
	"reflect"
	"testing"
)

func want(t *testing.T, label string, v string, ok bool, wv string, wok bool) {
	t.Helper()
	if v != wv || ok != wok {
		t.Errorf("%s = %q, %v; want %q, %v", label, v, ok, wv, wok)
	}
}

func TestStore_basic_reads(t *testing.T) {
	s := NewStore()
	s.Put("a", "1")
	s.Put("b", "2")
	s.Put("a", "3")
	s.Delete("b")
	v, ok := s.Get("a")
	want(t, "Get(a)", v, ok, "3", true)
	v, ok = s.Get("b")
	want(t, "Get(b) after delete", v, ok, "", false)
	v, ok = s.Get("zzz")
	want(t, "Get(zzz)", v, ok, "", false)
	if s.Versions() != 4 {
		t.Errorf("Versions = %d; want 4 (nothing is overwritten)", s.Versions())
	}
	s.Delete("never-existed")
	if s.Versions() != 5 {
		t.Errorf("Versions = %d after deleting a missing key; want 5", s.Versions())
	}
}

func TestSnapshot_is_frozen(t *testing.T) {
	s := NewStore()
	s.Put("k", "a")
	s.Put("gone", "x")
	snap := s.Snapshot()
	s.Put("k", "b")
	s.Delete("gone")
	s.Put("new", "n")
	v, ok := snap.Get("k")
	want(t, "snap.Get(k)", v, ok, "a", true)
	v, ok = snap.Get("gone")
	want(t, "snap.Get(gone)", v, ok, "x", true)
	v, ok = snap.Get("new")
	want(t, "snap.Get(new)", v, ok, "", false)
	v, ok = s.Get("k")
	want(t, "store.Get(k)", v, ok, "b", true)
	v, ok = s.Get("gone")
	want(t, "store.Get(gone)", v, ok, "", false)
}

func TestSnapshots_at_different_times(t *testing.T) {
	s := NewStore()
	var snaps []*Snapshot
	for i := 1; i <= 5; i++ {
		s.Put("counter", fmt.Sprint(i))
		snaps = append(snaps, s.Snapshot())
	}
	s.Delete("counter")
	for i, sn := range snaps {
		v, ok := sn.Get("counter")
		want(t, fmt.Sprintf("snapshot %d Get", i+1), v, ok, fmt.Sprint(i+1), true)
	}
	emptyStore := NewStore()
	early := emptyStore.Snapshot()
	emptyStore.Put("a", "1")
	if _, ok := early.Get("a"); ok {
		t.Errorf("a snapshot of an empty store sees a later write")
	}
}

func TestSnapshot_Keys(t *testing.T) {
	s := NewStore()
	s.Put("c", "1")
	s.Put("a", "1")
	s.Put("b", "1")
	s.Delete("b")
	snap1 := s.Snapshot()
	s.Put("d", "1")
	s.Put("b", "back")
	s.Delete("a")
	snap2 := s.Snapshot()
	if got := snap1.Keys(); !reflect.DeepEqual(got, []string{"a", "c"}) {
		t.Errorf("snap1.Keys = %v; want [a c]", got)
	}
	if got := snap2.Keys(); !reflect.DeepEqual(got, []string{"b", "c", "d"}) {
		t.Errorf("snap2.Keys = %v; want [b c d]", got)
	}
	if got := NewStore().Snapshot().Keys(); got == nil || len(got) != 0 {
		t.Errorf("Keys of an empty snapshot = %#v; want empty, non-nil", got)
	}
}

func TestPrune_without_snapshots_keeps_only_the_present(t *testing.T) {
	s := NewStore()
	for i := 0; i < 10; i++ {
		s.Put("a", fmt.Sprint(i))
		s.Put("b", fmt.Sprint(i))
	}
	s.Delete("b")
	s.Put("c", "only")
	s.Delete("d")
	s.Prune()
	if s.Versions() != 2 {
		t.Errorf("Versions = %d after Prune; want 2 (a and c; deleted keys leave no trace)", s.Versions())
	}
	v, ok := s.Get("a")
	want(t, "Get(a)", v, ok, "9", true)
	v, ok = s.Get("c")
	want(t, "Get(c)", v, ok, "only", true)
	if _, ok := s.Get("b"); ok {
		t.Errorf("a deleted key reappeared after Prune")
	}
}

func TestPrune_keeps_what_a_live_snapshot_needs(t *testing.T) {
	s := NewStore()
	s.Put("k", "v1")
	s.Put("k", "v2")
	s.Put("gone", "g")
	old := s.Snapshot()
	s.Put("k", "v3")
	s.Delete("gone")
	s.Put("k", "v4")
	mid := s.Snapshot()
	s.Put("k", "v5")

	s.Prune()
	// k: v2 (needed by old), v3 (needed by nobody but is newer than old), v4, v5 stay; v1 goes.
	if got := s.Versions(); got != 4+2 {
		t.Errorf("Versions = %d; want 6 (k: v2 v3 v4 v5, gone: its value and tombstone)", got)
	}
	v, ok := old.Get("k")
	want(t, "old.Get(k)", v, ok, "v2", true)
	v, ok = old.Get("gone")
	want(t, "old.Get(gone)", v, ok, "g", true)
	v, ok = mid.Get("k")
	want(t, "mid.Get(k)", v, ok, "v4", true)
	if _, ok := mid.Get("gone"); ok {
		t.Errorf("mid sees a deleted key")
	}
	v, ok = s.Get("k")
	want(t, "Get(k)", v, ok, "v5", true)

	old.Close()
	s.Prune()
	// Oldest live snapshot is now mid (after v4): k keeps v4, v5; "gone" is deleted for everyone.
	if got := s.Versions(); got != 2 {
		t.Errorf("Versions = %d after closing the old snapshot; want 2 (k: v4, v5)", got)
	}
	v, ok = mid.Get("k")
	want(t, "mid.Get(k) after prune", v, ok, "v4", true)

	mid.Close()
	mid.Close()
	s.Prune()
	if got := s.Versions(); got != 1 {
		t.Errorf("Versions = %d with no snapshots; want 1", got)
	}
	v, ok = s.Get("k")
	want(t, "Get(k) at the end", v, ok, "v5", true)
}

func TestPrune_never_changes_what_snapshots_see(t *testing.T) {
	rng := rand.New(rand.NewSource(5))
	s := NewStore()
	type frozen struct {
		snap  *Snapshot
		state map[string]string
	}
	model := map[string]string{}
	var open []frozen
	keys := []string{"a", "b", "c", "d", "e", "f"}
	for step := 0; step < 400; step++ {
		k := keys[rng.Intn(len(keys))]
		switch rng.Intn(6) {
		case 0:
			s.Delete(k)
			delete(model, k)
		case 1:
			cp := map[string]string{}
			for mk, mv := range model {
				cp[mk] = mv
			}
			open = append(open, frozen{s.Snapshot(), cp})
		case 2:
			if len(open) > 0 {
				i := rng.Intn(len(open))
				open[i].snap.Close()
				open = append(open[:i], open[i+1:]...)
			}
		case 3:
			s.Prune()
		default:
			v := fmt.Sprint(step)
			s.Put(k, v)
			model[k] = v
		}
		for _, f := range open {
			for _, key := range keys {
				v, ok := f.snap.Get(key)
				ev, eok := f.state[key]
				if v != ev || ok != eok {
					t.Fatalf("step %d: snapshot read %s = %q, %v; want %q, %v", step, key, v, ok, ev, eok)
				}
			}
		}
		for _, key := range keys {
			v, ok := s.Get(key)
			ev, eok := model[key]
			if v != ev || ok != eok {
				t.Fatalf("step %d: Get(%s) = %q, %v; want %q, %v", step, key, v, ok, ev, eok)
			}
		}
	}
}
