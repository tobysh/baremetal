package main

import (
	"reflect"
	"testing"
)

func TestRecoveryEveryCut(t *testing.T) {
	records := []Record{{0, 7}, {2, 9}, {0, 11}, {3, -1}}
	for cut := 0; cut <= len(records); cut++ {
		d := &Disk{Home: []int{1, 2, 3, 4}, Journal: Journal{true, append([]Record(nil), records...)}, FailAt: cut}
		e := Recover(d)
		if cut < len(records) {
			if e != ErrCrash || !d.Journal.Committed || !reflect.DeepEqual(d.Journal.Records, records) {
				t.Fatal("lost journal", cut, e)
			}
			d.FailAt = -1
			if e := Recover(d); e != nil {
				t.Fatal(e)
			}
		} else if e != nil {
			t.Fatal(e)
		}
		if !reflect.DeepEqual(d.Home, []int{11, 2, 9, -1}) || d.Journal.Committed || len(d.Journal.Records) != 0 {
			t.Fatal("recovery", cut, d)
		}
		attempts := d.Attempts
		if Recover(d) != nil || d.Attempts != attempts {
			t.Fatal("not idempotent")
		}
	}
}
func TestUncommittedAndInvalid(t *testing.T) {
	for _, committed := range []bool{false, true} {
		d := &Disk{Home: []int{1, 2}, Journal: Journal{committed, []Record{{0, 9}, {2, 8}}}, FailAt: -1}
		e := Recover(d)
		if committed && e != ErrJournal {
			t.Fatal("invalid")
		}
		if !committed && e != nil {
			t.Fatal("uncommitted")
		}
		if d.Attempts != 0 || !reflect.DeepEqual(d.Home, []int{1, 2}) {
			t.Fatal("partial validation")
		}
	}
}
