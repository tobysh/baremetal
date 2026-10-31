package main

import (
	"encoding/binary"
	"hash/crc32"
	"testing"
)

type flags struct {
	ignoreAppendErr bool // acknowledges a put whose append failed
	retryWhole      bool // on a short write, appends the whole record again
	noTruncate      bool // recovery leaves the damaged tail in place
	panicHeader     bool // recovery reads a header without checking its length
	noCRC           bool // no checksum; accepts truncated values
	split           bool // writes header and payload as two appends (correct)
}

type tstore struct {
	d    *FaultDisk
	m    map[string]string
	size int
	f    flags
}

func openWith(f flags) Opener {
	return func(d *FaultDisk) Engine {
		s := &tstore{d: d, m: map[string]string{}, f: f}
		b := d.Bytes()
		pos := 0
		for {
			if f.panicHeader {
				if pos >= len(b) {
					break
				}
				_ = binary.BigEndian.Uint32(b[pos:]) // panics on a 1-3 byte tail
			}
			if len(b)-pos < 8 {
				break
			}
			n := int(binary.BigEndian.Uint32(b[pos:]))
			if n < 1 {
				break
			}
			var payload []byte
			if f.noCRC {
				end := pos + 8 + n
				if end > len(b) {
					end = len(b)
				}
				payload = b[pos+8 : end]
			} else {
				if n > len(b)-pos-8 {
					break
				}
				payload = b[pos+8 : pos+8+n]
				if crc32.ChecksumIEEE(payload) != binary.BigEndian.Uint32(b[pos+4:]) {
					break
				}
			}
			kl := int(payload[0])
			if kl+1 > len(payload) {
				break
			}
			s.m[string(payload[1:1+kl])] = string(payload[1+kl:])
			pos += 8 + n
			if f.noCRC && pos > len(b) {
				pos = len(b)
				break
			}
		}
		s.size = pos
		if !f.noTruncate {
			d.Truncate(pos)
		}
		return s
	}
}

func (s *tstore) appendRec(rec []byte) error {
	parts := [][]byte{rec}
	if s.f.split {
		parts = [][]byte{rec[:8], rec[8:]}
	}
	for _, p := range parts {
		n, err := s.d.Append(p)
		if err != nil {
			if s.f.retryWhole && n > 0 {
				if _, err2 := s.d.Append(p); err2 == nil {
					continue
				}
			}
			if n > 0 {
				s.d.Truncate(s.size)
			}
			return err
		}
	}
	return nil
}

func (s *tstore) Put(key, value string) error {
	rec := encodeRecord(key, value)
	if err := s.appendRec(rec); err != nil && !s.f.ignoreAppendErr {
		return err
	}
	s.size += len(rec)
	s.m[key] = value
	return nil
}

func (s *tstore) Get(key string) (string, bool) { v, ok := s.m[key]; return v, ok }

var putSets = [][]KV{
	{{"a", "alpha"}, {"b", "bravo-value"}, {"a", "alpha-two"}, {"c", "charlie"}},
	{{"k1", "v-one"}, {"k2", "a much longer second value"}, {"k3", "x"}, {"k1", "v-uno"}, {"k4", "last one"}},
	{{"solo", "only write here"}},
}

func TestChecker_no_false_alarms_on_correct_engines(t *testing.T) {
	engines := map[string]Opener{
		"log":   OpenLog,
		"split": openWith(flags{split: true}),
		"copy":  openWith(flags{}),
	}
	for name, open := range engines {
		for i, puts := range putSets {
			if v := CheckEngine(open, puts); len(v) != 0 {
				t.Errorf("%s engine, put set %d: %d false violations, first: %s", name, i, len(v), v[0])
			}
		}
	}
}

func TestChecker_finds_each_broken_engine(t *testing.T) {
	broken := []struct {
		name string
		f    flags
		why  string
	}{
		{"acknowledges failed appends", flags{ignoreAppendErr: true}, "a put that returned nil was lost"},
		{"retries the whole record after a short write", flags{retryWhole: true}, "a torn prefix before the retry hides the record"},
		{"does not truncate the damaged tail", flags{noTruncate: true}, "writes after recovery are unreachable"},
		{"reads a header without a length check", flags{panicHeader: true}, "recovery panics on a short tail"},
		{"has no checksum and accepts truncated values", flags{noCRC: true}, "an invented, truncated value appears"},
		{"combines two faults: no truncate, split writes", flags{noTruncate: true, split: true}, "same class, different op count"},
	}
	for _, b := range broken {
		for i, puts := range putSets {
			if v := CheckEngine(openWith(b.f), puts); len(v) == 0 {
				t.Errorf("engine that %s passed put set %d unnoticed (%s)", b.name, i, b.why)
			}
		}
	}
}

func TestChecker_reports_are_deterministic(t *testing.T) {
	a := CheckEngine(openWith(flags{noTruncate: true}), putSets[0])
	b := CheckEngine(openWith(flags{noTruncate: true}), putSets[0])
	if len(a) != len(b) {
		t.Fatalf("two runs reported %d and %d violations", len(a), len(b))
	}
	for i := range a {
		if a[i] != b[i] {
			t.Fatalf("violation %d differs between runs", i)
		}
	}
}

func TestChecker_survives_a_broken_open(t *testing.T) {
	v := CheckEngine(openWith(flags{panicHeader: true}), putSets[1])
	if len(v) == 0 {
		t.Fatal("a panicking recovery must be reported, not crash the checker")
	}
}
