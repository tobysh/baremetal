package main

import (
	"bytes"
	"encoding/binary"
	"hash/crc32"
	"hash/fnv"
	"reflect"
	"testing"
)

func advCapsule() []byte {
	code := []byte{1, 5, 'h', 'e', 'l', 'l', 'o', 2, 4, 'n', 'o', 't', 'e', 3, 9, 0, 1, 2, 'o', 'k', 0}
	b := make([]byte, 12+len(code))
	copy(b, "BML1")
	binary.LittleEndian.PutUint16(b[6:8], uint16(len(code)))
	binary.LittleEndian.PutUint32(b[8:12], crc32.ChecksumIEEE(code))
	copy(b[12:], code)
	return b
}
func advConsistent(b []byte) bool {
	return len(b) >= 12 && string(b[:4]) == "BML1" && int(binary.LittleEndian.Uint16(b[6:8])) == len(b)-12 &&
		crc32.ChecksumIEEE(b[12:]) == binary.LittleEndian.Uint32(b[8:12])
}

func TestSeedForMatchesFNV1a(t *testing.T) {
	seen := map[uint64]bool{}
	for _, name := range []string{"", "TestLoader", "TestLoader2", "café"} {
		for _, idx := range []int{0, 1, 2, 255, 1 << 40, -1} {
			h := fnv.New64a()
			h.Write([]byte(name))
			var le [8]byte
			binary.LittleEndian.PutUint64(le[:], uint64(idx))
			h.Write(le[:])
			got := SeedFor(name, idx)
			if got != h.Sum64() || seen[got] {
				t.Fatal(name, idx, got, h.Sum64())
			}
			seen[got] = true
		}
	}
}

func TestMutantsAreStableAndIndependent(t *testing.T) {
	valid := advCapsule()
	orig := append([]byte{}, valid...)
	a, err := Mutants(valid, 42, 300)
	b, _ := Mutants(valid, 42, 300)
	c, _ := Mutants(valid, 43, 300)
	if err != nil || len(a) != 300 || !reflect.DeepEqual(a, b) || reflect.DeepEqual(a, c) {
		t.Fatal("seeded determinism", err, len(a))
	}
	if !bytes.Equal(valid, orig) {
		t.Fatal("valid capsule modified")
	}
	distinct := map[string]bool{}
	for i, m := range a {
		if bytes.Equal(m, valid) {
			t.Fatal("mutant equals the original", i)
		}
		distinct[string(m)] = true
	}
	if len(distinct) < 150 {
		t.Fatal("too little variety", len(distinct))
	}
	a[0][0] ^= 0xff
	if reflect.DeepEqual(a, b) == false && !bytes.Equal(b[0], func() []byte { x, _ := Mutants(valid, 42, 1); return x[0] }()) {
		t.Fatal("mutants share memory")
	}
	small, _ := Mutants(valid, 42, 10)
	if !reflect.DeepEqual(small, b[:10]) {
		t.Fatal("a shorter request must be a prefix of a longer one")
	}
}

func TestMutantsReachPastOuterChecks(t *testing.T) {
	valid := advCapsule()
	for _, seed := range []uint64{SeedFor("reach", 0), SeedFor("reach", 1), 7} {
		ms, _ := Mutants(valid, seed, 300)
		resized, consistent, rejectedInside, header := 0, 0, 0, 0
		for _, m := range ms {
			if len(m) != len(valid) {
				resized++
			}
			if advConsistent(m) {
				consistent++
				if _, err := LoadCapsule(m); err != nil {
					rejectedInside++
				}
			}
			if len(m) >= 12 && !bytes.Equal(m[:12], valid[:12]) {
				header++
			}
			LoadCapsule(m) // must never panic
		}
		if resized < 60 || consistent < 40 || rejectedInside < 5 || header < 100 {
			t.Fatal("generator is too weak", seed, resized, consistent, rejectedInside, header)
		}
	}
}

func TestMutantsRejectBadRequests(t *testing.T) {
	for _, c := range []struct {
		v []byte
		n int
	}{{advCapsule()[:11], 5}, {nil, 5}, {advCapsule(), 0}, {advCapsule(), -1}, {advCapsule(), 513}} {
		if got, err := Mutants(c.v, 1, c.n); err == nil || got != nil {
			t.Fatal("bad request accepted", len(c.v), c.n)
		}
	}
	header := advCapsule()[:12]
	header[6], header[7] = 0, 0
	if ms, err := Mutants(header, 3, 40); err != nil || len(ms) != 40 {
		t.Fatal("empty code part", err)
	} else {
		for _, m := range ms {
			if bytes.Equal(m, header) {
				t.Fatal("unchanged mutant")
			}
		}
	}
}

func oneMinimal(t *testing.T, got []byte, fails func([]byte) bool) {
	t.Helper()
	if !fails(append([]byte{}, got...)) {
		t.Fatal("result no longer fails", got)
	}
	for i := range got {
		cand := append(append([]byte{}, got[:i]...), got[i+1:]...)
		if fails(cand) {
			t.Fatal("not 1-minimal, byte removable at", i, got)
		}
	}
}

func TestMinimizeShrinksToTheCause(t *testing.T) {
	r := Rand{S: 99}
	noise := make([]byte, 240)
	for i := range noise {
		noise[i] = byte(r.Next()%200) + 20
	}
	subseq := func(b []byte) bool {
		seen := false
		for _, v := range b {
			if v == 7 {
				seen = true
			} else if v == 9 && seen {
				return true
			}
		}
		return false
	}
	in := append(append(append(append([]byte{}, noise[:100]...), 7), noise[100:200]...), append([]byte{9}, noise[200:]...)...)
	before := append([]byte{}, in...)
	calls := 0
	scribble := func(b []byte) bool {
		calls++
		ok := subseq(b)
		for i := range b {
			b[i] = 0
		}
		return ok
	}
	got, err := Minimize(in, scribble)
	if err != nil || !bytes.Equal(got, []byte{7, 9}) || !bytes.Equal(in, before) {
		t.Fatal(got, err)
	}
	if calls > len(in)*len(in) {
		t.Fatal("too many predicate calls", calls)
	}
	needle := func(b []byte) bool { return bytes.Contains(b, []byte{1, 2, 3}) && len(b) >= 5 }
	got, err = Minimize(append(append([]byte{50, 60}, 1, 2, 3), noise[:60]...), needle)
	if err != nil || len(got) != 5 {
		t.Fatal(got, err)
	}
	oneMinimal(t, got, needle)
	first := func(b []byte) bool { return len(b) > 0 && b[0] == 5 }
	got, _ = Minimize([]byte{5, 1, 1, 1, 1, 1, 1, 1, 1}, first)
	if !bytes.Equal(got, []byte{5}) {
		t.Fatal(got)
	}
}

func TestMinimizeNonFailingAndWholeInput(t *testing.T) {
	if got, err := Minimize([]byte{1, 2, 3}, func([]byte) bool { return false }); err == nil || got != nil {
		t.Fatal("passing input must be an error")
	}
	always := func([]byte) bool { return true }
	got, err := Minimize([]byte{1, 2, 3, 4}, always)
	if err != nil || len(got) != 0 {
		t.Fatal("anything fails, so the empty input is the minimal one", got, err)
	}
	need := func(b []byte) bool { return len(b) == 3 }
	got, _ = Minimize([]byte{1, 2, 3}, need)
	if !bytes.Equal(got, []byte{1, 2, 3}) {
		t.Fatal(got)
	}
	// Minimising a loader failure end to end.
	valid := advCapsule()
	ms, _ := Mutants(valid, SeedFor("e2e", 0), 200)
	for _, m := range ms {
		if advConsistent(m) {
			if _, err := LoadCapsule(m); err == nil {
				continue
			}
			rejects := func(b []byte) bool { _, err := LoadCapsule(b); return err != nil }
			min, err := Minimize(m, rejects)
			if err != nil || len(min) > len(m) {
				t.Fatal(err)
			}
			oneMinimal(t, min, rejects)
			return
		}
	}
	t.Fatal("no header-consistent rejected mutant to minimise")
}
