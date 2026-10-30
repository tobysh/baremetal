package main

import (
	"reflect"
	"testing"
)

func cloneMap(m map[string]string) map[string]string {
	out := map[string]string{}
	for k, v := range m {
		out[k] = v
	}
	return out
}
func cloneHost(h *Host) *Host {
	return &Host{Files: cloneMap(h.Files), Keys: cloneMap(h.Keys), Tasks: cloneMap(h.Tasks)}
}
func cloneJournal(j *Journal) *Journal {
	return &Journal{Entries: append([]Entry(nil), j.Entries...)}
}

func baselineHost() *Host {
	return &Host{
		Files: map[string]string{"notes.txt": "original", "keep.txt": "keep"},
		Keys:  map[string]string{"mode": "off"},
		Tasks: map[string]string{},
	}
}

var lifecycleSets = [][]Step{
	{{"file", "dropped.bin", "x"}, {"key", "mode", "on"}, {"task", "tidy", "daily"}},
	{{"file", "notes.txt", "one"}, {"file", "notes.txt", "two"}, {"file", "notes.txt", "three"}},
	{{"key", "fresh", "a"}, {"key", "fresh", "b"}, {"file", "keep.txt", ""}, {"task", "t", "v"}, {"key", "mode", "off"}},
	{{"task", "only", "step"}},
}

func TestEveryInterruptionPointIsRecoverable(t *testing.T) {
	for si, steps := range lifecycleSets {
		for crash := -1; crash <= 2*len(steps)+1; crash++ {
			h := baselineHost()
			base := baselineHost()
			j := &Journal{}
			crashed, err := Apply(h, j, steps, crash)
			wantCrash := crash >= 0 && crash <= 2*len(steps)
			if err != nil || crashed != wantCrash {
				t.Fatal("set", si, "crash", crash, crashed, err)
			}
			// A restarted process only has the persisted copies.
			restartedHost, restartedJournal := cloneHost(h), cloneJournal(j)
			if err := Recover(restartedHost, restartedJournal); err != nil {
				t.Fatal(err)
			}
			if d := Audit(restartedHost, base); len(d) != 0 || len(restartedJournal.Entries) != 0 {
				t.Fatal("set", si, "crash", crash, "leftovers", d, restartedJournal.Entries)
			}
			if err := Recover(restartedHost, restartedJournal); err != nil || len(Audit(restartedHost, base)) != 0 {
				t.Fatal("second recovery changed state")
			}
		}
	}
}

func TestJournalIsWrittenBeforeTheChange(t *testing.T) {
	steps := lifecycleSets[0]
	for crash := 0; crash <= 2*len(steps); crash++ {
		h := baselineHost()
		j := &Journal{}
		if crashed, err := Apply(h, j, steps, crash); !crashed || err != nil {
			t.Fatal(crash, crashed, err)
		}
		if len(j.Entries) != (crash+1)/2 {
			t.Fatal("crash", crash, "journal entries", len(j.Entries))
		}
		applied := 0
		for _, s := range steps {
			if v, ok := table(h, s.Kind)[s.Name]; ok && v == s.Value {
				applied++
			}
		}
		if applied != crash/2 {
			t.Fatal("crash", crash, "applied", applied)
		}
	}
	h := baselineHost()
	j := &Journal{}
	Apply(h, j, []Step{{"file", "notes.txt", "new"}}, 1)
	want := []Entry{{"file", "notes.txt", true, "original"}}
	if !reflect.DeepEqual(j.Entries, want) {
		t.Fatal("entry must record the previous state", j.Entries)
	}
}

func table(h *Host, kind string) map[string]string {
	switch kind {
	case "file":
		return h.Files
	case "key":
		return h.Keys
	}
	return h.Tasks
}

func TestCompletedRunLeavesJournalForTeardown(t *testing.T) {
	h, base, j := baselineHost(), baselineHost(), &Journal{}
	crashed, err := Apply(h, j, lifecycleSets[2], -1)
	if crashed || err != nil || len(j.Entries) != 5 {
		t.Fatal(crashed, err, j.Entries)
	}
	got := Audit(h, base)
	want := []string{"files/keep.txt changed", "keys/fresh added", "tasks/t added"}
	if !reflect.DeepEqual(got, want) {
		t.Fatal(got)
	}
	if err := Recover(h, j); err != nil || len(Audit(h, base)) != 0 {
		t.Fatal(err, Audit(h, base))
	}
	if _, err := Apply(h, j, lifecycleSets[3], -1); err != nil {
		t.Fatal("journal should be reusable after recovery", err)
	}
}

func TestAuditReportsEveryDifference(t *testing.T) {
	a := &Host{Files: map[string]string{"a": "1", "b": "2"}, Keys: map[string]string{"k": "v"}}
	b := &Host{Files: map[string]string{"a": "1", "b": "3", "c": "4"}, Tasks: map[string]string{"t": "x"}}
	got := Audit(a, b)
	want := []string{"files/b changed", "files/c removed", "keys/k added", "tasks/t removed"}
	if !reflect.DeepEqual(got, want) {
		t.Fatal(got)
	}
	empty := Audit(&Host{}, &Host{})
	if empty == nil || len(empty) != 0 {
		t.Fatal("clean audit must be an empty non-nil slice", empty)
	}
	if len(Audit(&Host{Files: map[string]string{}}, &Host{})) != 0 {
		t.Fatal("nil and empty maps are equal")
	}
}

func TestApplyAndRecoverRejectBadInput(t *testing.T) {
	long := make([]byte, 65)
	for i := range long {
		long[i] = 'n'
	}
	big := make([]byte, 257)
	many := make([]Step, 33)
	for i := range many {
		many[i] = Step{"file", "f", "v"}
	}
	bad := [][]Step{nil, {{"disk", "a", "v"}}, {{"file", "", "v"}}, {{"file", string(long), "v"}}, {{"file", "a", string(big)}}, {{"file", "a", "ok"}, {"bogus", "b", "v"}}, many}
	for i, steps := range bad {
		h, base, j := baselineHost(), baselineHost(), &Journal{}
		crashed, err := Apply(h, j, steps, -1)
		if crashed || err == nil || len(j.Entries) != 0 || len(Audit(h, base)) != 0 {
			t.Fatal("bad steps accepted", i)
		}
	}
	h, j := baselineHost(), &Journal{}
	Apply(h, j, lifecycleSets[3], 2)
	if _, err := Apply(h, j, lifecycleSets[3], -1); err == nil {
		t.Fatal("must refuse to run over an unrecovered journal")
	}
	if _, err := Apply(nil, &Journal{}, lifecycleSets[3], -1); err == nil {
		t.Fatal("nil host")
	}
	if _, err := Apply(h, nil, lifecycleSets[3], -1); err == nil {
		t.Fatal("nil journal")
	}
	h2, base := baselineHost(), baselineHost()
	j2 := &Journal{}
	Apply(h2, j2, lifecycleSets[0], -1)
	j2.Entries[1].Kind = "bogus"
	after := cloneHost(h2)
	if err := Recover(h2, j2); err == nil || !reflect.DeepEqual(after, h2) || len(j2.Entries) != 3 {
		t.Fatal("corrupt journal must be rejected without partial undo")
	}
	_ = base
	if err := Recover(nil, j2); err == nil {
		t.Fatal("nil host")
	}
}
