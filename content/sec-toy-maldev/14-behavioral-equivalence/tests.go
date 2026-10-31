package main

import (
	"bytes"
	"crypto/sha256"
	"encoding/binary"
	"encoding/hex"
	"fmt"
	"hash/crc32"
	"testing"
)

func eqCapsule(code []byte, entry uint16) []byte {
	b := make([]byte, 12+len(code))
	copy(b, "BML1")
	binary.LittleEndian.PutUint16(b[4:6], entry)
	binary.LittleEndian.PutUint16(b[6:8], uint16(len(code)))
	binary.LittleEndian.PutUint32(b[8:12], crc32.ChecksumIEEE(code))
	copy(b[12:], code)
	return b
}
func eqPackage(plain []byte, key byte) []byte {
	b := make([]byte, 11+len(plain))
	copy(b, "BX01")
	binary.LittleEndian.PutUint16(b[4:6], uint16(len(plain)))
	b[6] = key
	binary.LittleEndian.PutUint32(b[7:11], crc32.ChecksumIEEE(plain))
	for i, v := range plain {
		b[11+i] = v ^ key
	}
	return b
}
func eqEmit(s string) []byte { return append([]byte{1, byte(len(s))}, s...) }
func eqRead(s string) []byte { return append([]byte{2, byte(len(s))}, s...) }
func eqWait(n uint16) []byte { return []byte{3, byte(n), byte(n >> 8)} }
func eqCat(parts ...[]byte) []byte {
	return append(bytes.Join(parts, nil), 0)
}
func eqBundle(code []byte, entry uint16, key byte) []byte {
	return eqPackage(eqCapsule(code, entry), key)
}

// Independent reference encoding of the specification.
func eqRefKey(outs, reads []string, ticks uint32) string {
	var b bytes.Buffer
	u := func(n int) { binary.Write(&b, binary.LittleEndian, uint32(n)) }
	for _, l := range [][]string{outs, reads} {
		u(len(l))
		for _, s := range l {
			u(len(s))
			b.WriteString(s)
		}
	}
	u(int(ticks))
	s := sha256.Sum256(b.Bytes())
	return hex.EncodeToString(s[:])
}

func TestTraceKeyIsCanonical(t *testing.T) {
	cases := []Trace{
		{Outputs: []string{"a", "bc"}, Reads: []string{"k"}, Ticks: 3},
		{Outputs: []string{"ab", "c"}, Reads: []string{"k"}, Ticks: 3},
		{Outputs: []string{"abc"}, Reads: []string{"k"}, Ticks: 3},
		{Outputs: []string{"a", "bc"}, Reads: []string{"k"}, Ticks: 4},
		{Outputs: []string{"a", "bc"}, Reads: []string{"j"}, Ticks: 3},
		{Outputs: []string{"a", "bc", "k"}, Reads: nil, Ticks: 3},
		{Outputs: []string{"\x00", ""}, Ticks: 0},
		{Outputs: []string{"", "\x00"}, Ticks: 0},
		{Outputs: []string{}, Reads: []string{}, Ticks: 0},
	}
	seen := map[string]int{}
	for i, c := range cases {
		k := TraceKey(c)
		if k != eqRefKey(c.Outputs, c.Reads, c.Ticks) {
			t.Fatal("encoding differs from the specification", i, k)
		}
		if j, dup := seen[k]; dup {
			t.Fatal("collision", i, j)
		}
		seen[k] = i
	}
	if TraceKey(Trace{}) != TraceKey(Trace{Outputs: []string{}, Reads: []string{}}) {
		t.Fatal("nil and empty slices must match")
	}
}

func TestEquivalentPackagingsAgree(t *testing.T) {
	caps := map[string]string{"k": "v\x00"}
	scen := []Scenario{{Caps: caps, MaxOps: 10, MaxTicks: 100}, {Caps: nil, MaxOps: 10, MaxTicks: 100}, {Caps: caps, MaxOps: 1, MaxTicks: 0}}
	body := eqCat(eqEmit("go"), eqRead("k"), eqWait(5))
	for key := byte(1); key < 12; key++ {
		a := eqBundle(body, 0, 3)
		b := eqBundle(body, 0, key)
		if ok, err := Equivalent(a, b, scen); !ok || err != nil {
			t.Fatal("different XOR key", key, ok, err)
		}
	}
	// Different unreachable prefixes, same suffix.
	suffix := eqCat(eqEmit("tail"), eqWait(2))
	p1 := append(eqEmit("one"), suffix...)
	p2 := append(append(eqRead("secret"), eqWait(9)...), suffix...)
	a, b := eqBundle(p1, uint16(len(eqEmit("one"))), 5), eqBundle(p2, uint16(len(eqRead("secret"))+3), 6)
	scen2 := []Scenario{{MaxOps: 3, MaxTicks: 2}, {Caps: map[string]string{"secret": "s"}, MaxOps: 3, MaxTicks: 2}}
	if ok, err := Equivalent(a, b, scen2); !ok || err != nil {
		t.Fatal("unreachable prefixes must not matter", ok, err)
	}
}

func TestEquivalenceIsRelativeToScenarios(t *testing.T) {
	whole := eqBundle(eqCat(eqWait(5), eqEmit("hi")), 0, 7)
	split := eqBundle(eqCat(eqWait(2), eqWait(3), eqEmit("hi")), 0, 9)
	loose := []Scenario{{MaxOps: 10, MaxTicks: 10}}
	tight := []Scenario{{MaxOps: 3, MaxTicks: 10}}
	if ok, _ := Equivalent(whole, split, loose); !ok {
		t.Fatal("split WAIT is equivalent when budget is ample")
	}
	if ok, _ := Equivalent(whole, split, tight); ok {
		t.Fatal("op budget exposes the split WAIT")
	}
	if ok, _ := Equivalent(whole, split, append(loose, tight...)); ok {
		t.Fatal("one differing scenario is enough")
	}
	// Differences visible only when a capability exists.
	a := eqBundle(eqCat(eqEmit("x")), 0, 3)
	b := eqBundle(eqCat(eqEmit("x"), eqRead("k")), 0, 3)
	absent := []Scenario{{MaxOps: 5, MaxTicks: 0}}
	present := []Scenario{{Caps: map[string]string{"k": "1"}, MaxOps: 5, MaxTicks: 0}}
	if ok, _ := Equivalent(a, b, present); ok {
		t.Fatal("extra read changes the trace")
	}
	if ok, _ := Equivalent(a, b, absent); ok {
		t.Fatal("rejected versus accepted is a difference")
	}
	// Different outputs with the same total bytes.
	c := eqBundle(eqCat(eqEmit("ab"), eqEmit("c")), 0, 3)
	d := eqBundle(eqCat(eqEmit("a"), eqEmit("bc")), 0, 3)
	if ok, _ := Equivalent(c, d, absent); ok {
		t.Fatal("output boundaries matter")
	}
}

func TestEquivalentTreatsRejectionAsBehavior(t *testing.T) {
	good := eqBundle(eqCat(eqEmit("x")), 0, 3)
	corrupt1 := append([]byte{}, good...)
	corrupt1[12] ^= 1
	corrupt2 := []byte("not a bundle at all")
	scen := []Scenario{{MaxOps: 5, MaxTicks: 5}}
	if ok, err := Equivalent(corrupt1, corrupt2, scen); !ok || err != nil {
		t.Fatal("two rejected artifacts agree", ok, err)
	}
	if ok, _ := Equivalent(good, corrupt1, scen); ok {
		t.Fatal("accepted versus rejected")
	}
	if ok, _ := Equivalent(good, good, []Scenario{{MaxOps: 0, MaxTicks: 0}}); !ok {
		t.Fatal("an unusable budget fails both equally")
	}
}

func TestEquivalentScenarioLimitsAndPurity(t *testing.T) {
	a := eqBundle(eqCat(eqRead("k")), 0, 3)
	for _, s := range [][]Scenario{nil, {}, make([]Scenario, 65)} {
		if ok, err := Equivalent(a, a, s); ok || err == nil {
			t.Fatal("scenario count limits", len(s))
		}
	}
	caps := map[string]string{"k": "v", "z": fmt.Sprint(1)}
	before := fmt.Sprint(caps)
	orig := append([]byte{}, a...)
	ok, err := Equivalent(a, a, []Scenario{{Caps: caps, MaxOps: 4, MaxTicks: 1}})
	if !ok || err != nil || fmt.Sprint(caps) != before || !bytes.Equal(orig, a) {
		t.Fatal("inputs must be left alone")
	}
}
