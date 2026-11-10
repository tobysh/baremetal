package main

import (
	"math/rand"
	"reflect"
	"testing"
)

func journal(committed bool, home []int, recs ...Record) *Disk {
	return &Disk{Home: append([]int(nil), home...), Journal: Journal{committed, append([]Record(nil), recs...)}, FailAt: -1}
}

func apply(home []int, recs []Record, n int) []int {
	out := append([]int(nil), home...)
	for _, r := range recs[:n] {
		out[r.Block] = r.Value
	}
	return out
}

func TestRecoveryEveryCut(t *testing.T) {
	records := []Record{{0, 7}, {2, 9}, {0, 11}, {3, -1}}
	home := []int{1, 2, 3, 4}
	for cut := 0; cut <= len(records); cut++ {
		d := journal(true, home, records...)
		d.FailAt = cut
		e := Recover(d)
		if cut < len(records) {
			if e != ErrCrash {
				t.Fatalf("cut %d: Recover = %v, want ErrCrash returned straight away", cut, e)
			}
			if d.Attempts != cut+1 {
				t.Fatalf("cut %d: %d write attempts, want %d (stop at the first error)", cut, d.Attempts, cut+1)
			}
			if !reflect.DeepEqual(d.Home, apply(home, records, cut)) {
				t.Fatalf("cut %d: Home = %v, want exactly the first %d records applied", cut, d.Home, cut)
			}
			if !d.Journal.Committed || !reflect.DeepEqual(d.Journal.Records, records) {
				t.Fatalf("cut %d: a failed recovery must keep the whole journal, found %+v", cut, d.Journal)
			}
			d.FailAt = -1
			if e := Recover(d); e != nil {
				t.Fatalf("cut %d: retry failed: %v", cut, e)
			}
		} else if e != nil {
			t.Fatal(e)
		}
		if !reflect.DeepEqual(d.Home, []int{11, 2, 9, -1}) || d.Journal.Committed || len(d.Journal.Records) != 0 {
			t.Fatalf("cut %d: after recovery Home = %v journal = %+v", cut, d.Home, d.Journal)
		}
		attempts := d.Attempts
		if Recover(d) != nil || d.Attempts != attempts {
			t.Fatal("a second Recover must make no writes")
		}
	}
}

func TestUncommittedJournalIgnored(t *testing.T) {
	recs := []Record{{0, 9}, {1, 8}}
	d := journal(false, []int{1, 2}, recs...)
	if e := Recover(d); e != nil {
		t.Fatalf("Recover = %v, want nil", e)
	}
	if d.Attempts != 0 || !reflect.DeepEqual(d.Home, []int{1, 2}) {
		t.Fatalf("an uncommitted transaction must not be applied: attempts %d, Home %v", d.Attempts, d.Home)
	}
	if d.Journal.Committed || !reflect.DeepEqual(d.Journal.Records, recs) {
		t.Fatalf("Recover must leave an uncommitted journal alone, found %+v", d.Journal)
	}
}

func TestInvalidAddressWritesNothing(t *testing.T) {
	bad := [][]Record{
		{{0, 9}, {2, 8}},
		{{2, 8}, {0, 9}},
		{{-1, 5}},
		{{0, 1}, {1, 2}, {-3, 4}, {0, 5}},
		{{0, 1}, {1, 2}, {99, 4}},
	}
	for i, recs := range bad {
		d := journal(true, []int{1, 2}, recs...)
		if e := Recover(d); e != ErrJournal {
			t.Fatalf("case %d: Recover = %v, want ErrJournal", i, e)
		}
		if d.Attempts != 0 || !reflect.DeepEqual(d.Home, []int{1, 2}) {
			t.Fatalf("case %d: validation must happen before any write (attempts %d, Home %v)", i, d.Attempts, d.Home)
		}
		if !d.Journal.Committed || !reflect.DeepEqual(d.Journal.Records, recs) {
			t.Fatalf("case %d: the journal must be left as it was, found %+v", i, d.Journal)
		}
	}
}

func TestDuplicateAddressesLastWins(t *testing.T) {
	d := journal(true, []int{0, 0, 0}, Record{1, 5}, Record{1, 6}, Record{2, 1}, Record{1, 7}, Record{2, 0})
	if e := Recover(d); e != nil {
		t.Fatal(e)
	}
	if !reflect.DeepEqual(d.Home, []int{0, 7, 0}) {
		t.Fatalf("Home = %v, want [0 7 0] (records applied in order)", d.Home)
	}
	if d.Attempts != 5 {
		t.Fatalf("every record is a write: %d attempts, want 5", d.Attempts)
	}
}

func TestEmptyCommittedJournal(t *testing.T) {
	d := journal(true, []int{4, 5})
	if e := Recover(d); e != nil || d.Journal.Committed || d.Attempts != 0 || !reflect.DeepEqual(d.Home, []int{4, 5}) {
		t.Fatalf("an empty committed journal is valid and must be cleared: %v %+v attempts %d", e, d.Journal, d.Attempts)
	}
}

func TestCrashDuringRecoveryRepeatedly(t *testing.T) {
	records := []Record{{3, 30}, {1, 10}, {3, 31}, {0, 5}, {4, 44}, {1, 11}}
	home := []int{9, 9, 9, 9, 9}
	d := journal(true, home, records...)
	// Each pass survives one more write than the last, then crashes again.
	for pass := 0; pass < len(records); pass++ {
		d.FailAt = d.Attempts + pass
		if e := Recover(d); e != ErrCrash {
			t.Fatalf("pass %d: want ErrCrash, got %v", pass, e)
		}
		if !d.Journal.Committed || len(d.Journal.Records) != len(records) {
			t.Fatalf("pass %d: the journal was lost during a failed recovery", pass)
		}
	}
	d.FailAt = -1
	if e := Recover(d); e != nil {
		t.Fatal(e)
	}
	if want := apply(home, records, len(records)); !reflect.DeepEqual(d.Home, want) {
		t.Fatalf("Home = %v, want %v", d.Home, want)
	}
	if d.FailAt != -1 {
		t.Fatal("Recover must not modify FailAt")
	}
}

func TestAttemptsCounterNotReset(t *testing.T) {
	d := journal(true, []int{0, 0}, Record{0, 1}, Record{1, 2})
	d.Attempts = 10
	d.FailAt = -1
	Recover(d)
	if d.Attempts != 12 {
		t.Fatalf("Attempts = %d, want 12: Recover must count on from the disk's own counter", d.Attempts)
	}
}

func TestRandomJournalsAndCrashPoints(t *testing.T) {
	r := rand.New(rand.NewSource(5))
	for trial := 0; trial < 300; trial++ {
		size := 1 + r.Intn(6)
		home := make([]int, size)
		for i := range home {
			home[i] = r.Intn(100)
		}
		recs := make([]Record, r.Intn(9))
		for i := range recs {
			recs[i] = Record{r.Intn(size), r.Intn(1000) - 500}
		}
		d := journal(true, home, recs...)
		crashes := r.Intn(4)
		for c := 0; c < crashes; c++ {
			d.FailAt = d.Attempts + r.Intn(len(recs)+1)
			e := Recover(d)
			if e != nil && e != ErrCrash {
				t.Fatalf("trial %d: unexpected error %v", trial, e)
			}
			if e == ErrCrash && (!d.Journal.Committed || len(d.Journal.Records) != len(recs)) {
				t.Fatalf("trial %d: journal damaged by a crashed recovery", trial)
			}
		}
		d.FailAt = -1
		if e := Recover(d); e != nil {
			t.Fatalf("trial %d: final recovery failed: %v", trial, e)
		}
		if want := apply(home, recs, len(recs)); !reflect.DeepEqual(d.Home, want) {
			t.Fatalf("trial %d: Home = %v, want %v after records %v", trial, d.Home, want, recs)
		}
		if d.Journal.Committed || len(d.Journal.Records) != 0 {
			t.Fatalf("trial %d: journal not cleared: %+v", trial, d.Journal)
		}
	}
}
