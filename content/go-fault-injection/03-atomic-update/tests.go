package main

import (
	"bytes"
	"errors"
	"testing"
)

var ErrCrash = errors.New("crash")

type crashDisk struct {
	files   map[string][]byte
	ops     int
	crashAt int // -1 never
	tear    int // 0 none, 1 one byte, 2 half, 3 all but one
	after   bool
	dead    bool
}

func newCrashDisk(crashAt, tear int, after bool) *crashDisk {
	return &crashDisk{files: map[string][]byte{}, crashAt: crashAt, tear: tear, after: after}
}

func (d *crashDisk) hit() (crash bool, dead bool) {
	if d.dead {
		return false, true
	}
	i := d.ops
	d.ops++
	return i == d.crashAt, false
}

func (d *crashDisk) WriteFile(name string, data []byte) error {
	crash, dead := d.hit()
	if dead {
		return ErrCrash
	}
	if crash {
		n := 0
		switch d.tear {
		case 1:
			n = 1
		case 2:
			n = len(data) / 2
		case 3:
			n = len(data) - 1
		}
		if n > len(data) {
			n = len(data)
		}
		if n < 0 {
			n = 0
		}
		d.files[name] = append([]byte{}, data[:n]...)
		d.dead = true
		return ErrCrash
	}
	d.files[name] = append([]byte{}, data...)
	return nil
}

func (d *crashDisk) Rename(o, n string) error {
	crash, dead := d.hit()
	if dead {
		return ErrCrash
	}
	if crash && !d.after {
		d.dead = true
		return ErrCrash
	}
	b, ok := d.files[o]
	if !ok {
		return errors.New("no such file")
	}
	d.files[n] = b
	delete(d.files, o)
	if crash {
		d.dead = true
		return ErrCrash
	}
	return nil
}

func (d *crashDisk) Remove(name string) error {
	crash, dead := d.hit()
	if dead {
		return ErrCrash
	}
	if crash && !d.after {
		d.dead = true
		return ErrCrash
	}
	delete(d.files, name)
	if crash {
		d.dead = true
		return ErrCrash
	}
	return nil
}

func (d *crashDisk) ReadFile(name string) ([]byte, bool) {
	b, ok := d.files[name]
	return append([]byte(nil), b...), ok
}

func (d *crashDisk) Reboot() { d.dead = false; d.crashAt = -1 }

var versions = [][]byte{[]byte("first version"), {}, []byte("a third, rather longer version of the data"), []byte("4")}

func TestSave_load_roundtrip_and_replace(t *testing.T) {
	d := newCrashDisk(-1, 0, false)
	if _, found, err := Load(d); found || err != nil {
		t.Fatalf("empty disk: found=%v err=%v", found, err)
	}
	for i, v := range versions {
		if err := Save(d, v); err != nil {
			t.Fatal(err)
		}
		got, found, err := Load(d)
		if err != nil || !found || !bytes.Equal(got, v) {
			t.Fatalf("version %d: got %q found=%v err=%v", i, got, found, err)
		}
	}
	if _, ok := d.ReadFile("db.tmp"); ok {
		t.Errorf("db.tmp left behind after successful saves")
	}
}

func TestLoad_reports_corruption_and_removes_stray_tmp(t *testing.T) {
	d := newCrashDisk(-1, 0, false)
	Save(d, []byte("good data"))
	raw, _ := d.ReadFile("db")
	raw[6] ^= 0xff
	d.files["db"] = raw
	if got, found, err := Load(d); err != ErrCorrupt || found || got != nil {
		t.Errorf("flipped byte: got %q found=%v err=%v, want ErrCorrupt", got, found, err)
	}
	d.files["db"] = raw[:5]
	if _, _, err := Load(d); err != ErrCorrupt {
		t.Errorf("short file: err=%v, want ErrCorrupt", err)
	}
	d.files["db.tmp"] = []byte("junk")
	Load(d)
	if _, ok := d.ReadFile("db.tmp"); ok {
		t.Errorf("Load left the stray db.tmp in place")
	}
}

func runSaves(d *crashDisk) (committed []byte, hasCommitted bool, inflight []byte, hasInflight bool) {
	for _, v := range versions {
		if err := Save(d, v); err != nil {
			return committed, hasCommitted, v, true
		}
		committed, hasCommitted = v, true
	}
	return
}

func TestCrash_at_every_operation_keeps_committed_data(t *testing.T) {
	clean := newCrashDisk(-1, 0, false)
	runSaves(clean)
	total := clean.ops
	if total < len(versions) {
		t.Fatalf("saving %d versions used only %d disk operations", len(versions), total)
	}
	checked := 0
	for at := 0; at < total; at++ {
		for tear := 0; tear < 4; tear++ {
			for _, after := range []bool{false, true} {
				d := newCrashDisk(at, tear, after)
				com, hasCom, inf, hasInf := runSaves(d)
				if !hasInf {
					continue
				}
				d.Reboot()
				got, found, err := Load(d)
				where := func() string {
					return "crash at op " + string(rune('0'+at/10)) + string(rune('0'+at%10))
				}
				if err != nil {
					t.Fatalf("%s tear %d after=%v: Load returned %v after a crash; a torn file must never be visible", where(), tear, after, err)
				}
				switch {
				case !found && hasCom:
					t.Fatalf("%s tear %d after=%v: committed version %q was lost", where(), tear, after, com)
				case found && !(hasCom && bytes.Equal(got, com)) && !(hasInf && bytes.Equal(got, inf)):
					t.Fatalf("%s tear %d after=%v: loaded %q, want the committed %q or the in-flight %q", where(), tear, after, got, com, inf)
				}
				if _, ok := d.ReadFile("db.tmp"); ok {
					t.Fatalf("%s tear %d after=%v: Load left db.tmp behind", where(), tear, after)
				}
				again, found2, err2 := Load(d)
				if err2 != nil || found2 != found || !bytes.Equal(again, got) {
					t.Fatalf("%s: a second Load changed the result (recovery is not idempotent)", where())
				}
				if err := Save(d, []byte("after recovery")); err != nil {
					t.Fatalf("%s: Save after recovery failed: %v", where(), err)
				}
				if g, f, e := Load(d); e != nil || !f || string(g) != "after recovery" {
					t.Fatalf("%s: after recovery and a new save, got %q found=%v err=%v", where(), g, f, e)
				}
				checked++
			}
		}
	}
	if checked < 20 {
		t.Errorf("only %d crash scenarios were exercised", checked)
	}
}

func TestSave_returns_the_error_and_stops(t *testing.T) {
	d := newCrashDisk(0, 2, false)
	if err := Save(d, []byte("data")); !errors.Is(err, ErrCrash) {
		t.Errorf("Save error = %v, want the disk's error", err)
	}
	d = newCrashDisk(1, 0, false) // rename lost
	if err := Save(d, []byte("data")); !errors.Is(err, ErrCrash) {
		t.Errorf("Save error on failed rename = %v", err)
	}
}
