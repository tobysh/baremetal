package main

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"hash/crc32"
	"reflect"
	"strings"
	"testing"
)

// Test-side model, written independently of the solution: it builds code from
// instruction descriptions and records offsets while it assembles.
type part struct {
	op   byte
	text string
	n    uint16
	sym  string
}

type built struct {
	code    []byte
	starts  []int
	exports map[string]int
	relocs  []Reloc
}

func build(parts ...part) built {
	b := built{exports: map[string]int{}}
	for _, p := range parts {
		b.starts = append(b.starts, len(b.code))
		switch p.op {
		case 0:
			b.code = append(b.code, 0)
		case 1, 2:
			b.code = append(b.code, p.op, byte(len(p.text)))
			b.code = append(b.code, p.text...)
		case 3:
			b.code = append(b.code, 3, byte(p.n), byte(p.n>>8))
		case 4:
			b.relocs = append(b.relocs, Reloc{Site: len(b.code) + 1, Symbol: p.sym})
			b.code = append(b.code, 4, 0, 0)
		}
	}
	return b
}

func (b built) export(name string, idx int) built {
	b.exports[name] = b.starts[idx]
	return b
}

func (b built) obj() Object {
	return Object{Code: append([]byte{}, b.code...), Exports: copyExports(b.exports), Relocs: append([]Reloc{}, b.relocs...)}
}

func copyExports(m map[string]int) map[string]int {
	if m == nil {
		return nil
	}
	out := map[string]int{}
	for k, v := range m {
		out[k] = v
	}
	return out
}

func capsule(magic string, entry int, code []byte) []byte {
	b := make([]byte, 12, 12+len(code))
	copy(b, magic)
	binary.LittleEndian.PutUint16(b[4:], uint16(entry))
	binary.LittleEndian.PutUint16(b[6:], uint16(len(code)))
	binary.LittleEndian.PutUint32(b[8:], crc32.ChecksumIEEE(code))
	return append(b, code...)
}

func cloneObjs(in []Object) []Object {
	if in == nil {
		return nil
	}
	out := make([]Object, len(in))
	for i, o := range in {
		out[i] = Object{Exports: copyExports(o.Exports)}
		if o.Code != nil {
			out[i].Code = append([]byte{}, o.Code...)
		}
		if o.Relocs != nil {
			out[i].Relocs = append([]Reloc{}, o.Relocs...)
		}
	}
	return out
}

func mustFail(t *testing.T, name string, objs []Object) {
	t.Helper()
	before := cloneObjs(objs)
	got, err := Link(objs)
	if err == nil || got != nil {
		t.Fatalf("%s: accepted (%x, %v)", name, got, err)
	}
	if !reflect.DeepEqual(before, objs) {
		t.Fatalf("%s: input modified", name)
	}
}

// goodSet: main calls f and g; lib has f and g; f is called twice.
func goodSet(i int) (main, lib built) {
	main = build(part{op: 4, sym: "f"}, part{op: 4, sym: "g"}, part{op: 4, sym: "f"}, part{op: 0}).export("main", 0)
	lib = build(part{op: 1, text: strings.Repeat("x", i)}, part{op: 0}, part{op: 3, n: uint16(i + 1)}, part{op: 0}).export("f", 0).export("g", 2)
	return
}

func TestLinkPatchesLinkedOffsets(t *testing.T) {
	for i := 0; i < 25; i++ {
		m, l := goodSet(i)
		// main first, then lib.
		objs := []Object{m.obj(), l.obj()}
		before := cloneObjs(objs)
		got, err := Link(objs)
		fOff := len(m.code) + l.starts[0]
		gOff := len(m.code) + l.starts[2]
		want := []byte{4, byte(fOff), byte(fOff >> 8), 4, byte(gOff), byte(gOff >> 8), 4, byte(fOff), byte(fOff >> 8), 0}
		want = append(want, l.code...)
		if err != nil || !bytes.Equal(got, capsule("BML2", 0, want)) {
			t.Fatalf("i=%d got %x err %v want %x", i, got, err, capsule("BML2", 0, want))
		}
		if !reflect.DeepEqual(before, objs) {
			t.Fatal("input modified")
		}
		for j := range got {
			got[j] ^= 0xff
		}
		if !reflect.DeepEqual(before, objs) {
			t.Fatal("output aliases input")
		}
		// lib first: bases swap, main entry moves.
		got, err = Link([]Object{l.obj(), m.obj()})
		base := len(l.code)
		fOff, gOff = l.starts[0], l.starts[2]
		want = append([]byte{}, l.code...)
		want = append(want, 4, byte(fOff), byte(fOff>>8), 4, byte(gOff), byte(gOff>>8), 4, byte(fOff), byte(fOff>>8), 0)
		if err != nil || !bytes.Equal(got, capsule("BML2", base, want)) {
			t.Fatalf("lib first i=%d got %x err %v", i, got, err)
		}
	}
}

func TestLinkSelfAndNestedCalls(t *testing.T) {
	// main -> a (in same object) -> b (in another object); a also calls itself.
	o1 := build(part{op: 4, sym: "a"}, part{op: 0}, part{op: 1, text: "A"}, part{op: 4, sym: "b"}, part{op: 4, sym: "a"}, part{op: 0}).export("main", 0).export("a", 2)
	o2 := build(part{op: 2, text: "key"}, part{op: 0}).export("b", 0)
	got, err := Link([]Object{o1.obj(), o2.obj()})
	b := len(o1.code)
	want := []byte{4, 4, 0, 0, 1, 1, 'A', 4, byte(b), 0, 4, 4, 0, 0, 2, 3, 'k', 'e', 'y', 0}
	if err != nil || !bytes.Equal(got, capsule("BML2", 0, want)) {
		t.Fatalf("got %x err %v", got, err)
	}
}

func TestLinkVersionSelection(t *testing.T) {
	// No CALL, one HALT: must be byte-identical to a version-1 capsule, entry mid-code.
	o := build(part{op: 1, text: "hi"}, part{op: 3, n: 258}, part{op: 0}).export("main", 1)
	got, err := Link([]Object{o.obj()})
	if err != nil || !bytes.Equal(got, capsule("BML1", 4, o.code)) {
		t.Fatalf("v1: %x %v", got, err)
	}
	if !validV1(got) {
		t.Fatal("version-1 output not accepted by an independent v1 validator")
	}
	// No CALL but two HALTs: v1 cannot express it.
	a := build(part{op: 1, text: "a"}, part{op: 0}).export("main", 0)
	b := build(part{op: 1, text: "b"}, part{op: 0}).export("other", 0)
	got, err = Link([]Object{a.obj(), b.obj()})
	if err != nil || string(got[:4]) != "BML2" || !bytes.Equal(got, capsule("BML2", 0, append(append([]byte{}, a.code...), b.code...))) {
		t.Fatalf("two halts: %x %v", got, err)
	}
	if validV1(got) {
		t.Fatal("v1 validator accepted BML2")
	}
	// A CALL always forces version 2, even with one HALT.
	c := build(part{op: 4, sym: "main"}, part{op: 0}).export("main", 0)
	got, err = Link([]Object{c.obj()})
	if err != nil || !bytes.Equal(got, capsule("BML2", 0, []byte{4, 0, 0, 0})) {
		t.Fatalf("self call: %x %v", got, err)
	}
}

// validV1 is the version-1 loader contract from lesson 4, reimplemented here.
func validV1(b []byte) bool {
	if len(b) < 12 || string(b[:4]) != "BML1" {
		return false
	}
	entry, n := int(binary.LittleEndian.Uint16(b[4:])), int(binary.LittleEndian.Uint16(b[6:]))
	if n == 0 || n > 4096 || len(b) != 12+n || entry >= n || crc32.ChecksumIEEE(b[12:]) != binary.LittleEndian.Uint32(b[8:]) {
		return false
	}
	code := b[12:]
	ok, count := false, 0
	for pc := 0; pc < len(code); {
		if pc == entry {
			ok = true
		}
		op := code[pc]
		pc++
		count++
		switch op {
		case 0:
			if pc != len(code) {
				return false
			}
		case 1, 2:
			if pc >= len(code) || int(code[pc]) > len(code)-pc-1 {
				return false
			}
			pc += 1 + int(code[pc])
		case 3:
			pc += 2
			if pc > len(code) {
				return false
			}
		default:
			return false
		}
	}
	return ok && count <= 64
}

func TestLinkRejectsBadRelocations(t *testing.T) {
	m, l := goodSet(3)
	if _, err := Link([]Object{m.obj(), l.obj()}); err != nil {
		t.Fatal("fixture should link", err)
	}
	n := len(m.code)
	sites := map[string]int{"negative": -1, "past end": n, "operand end": n - 1, "far": 1 << 20, "huge negative": -1 << 20, "at opcode": 0, "inside next call": 3, "last halt": n - 1}
	for name, s := range sites {
		mm := m.obj()
		mm.Relocs[0].Site = s
		mustFail(t, "site "+name, []Object{mm, l.obj()})
	}
	// Site inside a text operand of the lib object, then at its length byte.
	for _, s := range []int{0, 1, 2, 3} {
		ll := l.obj()
		ll.Relocs = append(ll.Relocs, Reloc{Site: s, Symbol: "f"})
		mustFail(t, fmt.Sprint("lib site ", s), []Object{m.obj(), ll})
	}
	// Unresolved symbol, unresolved CALL, duplicate relocation.
	mm := m.obj()
	mm.Relocs[1].Symbol = "nope"
	mustFail(t, "unresolved symbol", []Object{mm, l.obj()})
	mm = m.obj()
	mm.Relocs = mm.Relocs[:2]
	mustFail(t, "call without relocation", []Object{mm, l.obj()})
	mm = m.obj()
	mm.Relocs = append(mm.Relocs, mm.Relocs[0])
	mustFail(t, "duplicate relocation", []Object{mm, l.obj()})
	mm = m.obj()
	mm.Relocs[2] = mm.Relocs[0]
	mustFail(t, "second call unrelocated", []Object{mm, l.obj()})
	// Relocation attached to a non-call object.
	ll := l.obj()
	ll.Relocs = []Reloc{{Site: 1, Symbol: "f"}}
	mustFail(t, "reloc in call-free object", []Object{m.obj(), ll})
	// Nonzero placeholder.
	mm = m.obj()
	mm.Code[1] = 9
	mustFail(t, "nonzero placeholder", []Object{mm, l.obj()})
}

func TestLinkRejectsBadSymbols(t *testing.T) {
	m, l := goodSet(2)
	l2 := build(part{op: 0}).export("f", 0) // duplicate of lib's f
	mustFail(t, "duplicate across objects", []Object{m.obj(), l.obj(), l2.obj()})
	dm := m.obj()
	dm.Exports["g"] = 0 // main object also exports g, as does lib
	mustFail(t, "duplicate main/lib", []Object{dm, l.obj()})
	for name, off := range map[string]int{"neg": -1, "past end": len(l.code), "inside text": 1, "inside wait": l.starts[2] + 1, "huge": 1 << 30} {
		ll := l.obj()
		ll.Exports["f"] = off
		mustFail(t, "export "+name, []Object{m.obj(), ll})
	}
	for _, name := range []string{"", strings.Repeat("n", 33)} {
		ll := l.obj()
		ll.Exports[name] = 0
		mustFail(t, "export name "+fmt.Sprint(len(name)), []Object{m.obj(), ll})
	}
	// A 32-byte name is fine.
	ll := l.obj()
	ll.Exports[strings.Repeat("n", 32)] = 0
	if _, err := Link([]Object{m.obj(), ll}); err != nil {
		t.Fatal("32-byte name rejected", err)
	}
	// main missing; main exported at a bad offset.
	nm := m.obj()
	delete(nm.Exports, "main")
	mustFail(t, "no main", []Object{nm, l.obj()})
	nm = m.obj()
	nm.Exports["main"] = 1
	mustFail(t, "main inside operand", []Object{nm, l.obj()})
}

func TestLinkRejectsMalformedCodeAndLimits(t *testing.T) {
	ex := map[string]int{"main": 0}
	codes := map[string][]byte{
		"empty": {}, "no halt": {1, 0}, "unknown op": {9, 0}, "truncated emit": {1, 5, 'a', 0},
		"truncated wait": {3, 1}, "zero wait": {3, 0, 0, 0}, "empty read key": {2, 0, 0}, "truncated call": {4, 0}, "halt then junk": {0, 1},
	}
	for name, c := range codes {
		mustFail(t, name, []Object{{Code: c, Exports: ex}})
	}
	mustFail(t, "no objects", nil)
	var nine []Object
	for i := 0; i < 9; i++ {
		o := Object{Code: []byte{0}, Exports: map[string]int{}}
		if i == 0 {
			o.Exports["main"] = 0
		}
		nine = append(nine, o)
	}
	mustFail(t, "nine objects", nine)
	if _, err := Link(nine[:8]); err != nil {
		t.Fatal("eight objects rejected", err)
	}
	// Exactly 4096 bytes in 16 instructions passes, 4097 fails.
	mk := func(last int) Object {
		code := []byte{}
		for i := 0; i < 15; i++ {
			code = append(code, 1, 255)
			code = append(code, bytes.Repeat([]byte{'a'}, 255)...)
		}
		code = append(code, 1, byte(last))
		code = append(code, bytes.Repeat([]byte{'b'}, last)...)
		return Object{Code: append(code, 0), Exports: ex}
	}
	if got, err := Link([]Object{mk(238)}); err != nil || len(got) != 12+4096 {
		t.Fatal("4096 bytes rejected", err)
	}
	mustFail(t, "4097 bytes", []Object{mk(239)})
	// Each object small, total too large.
	half := Object{Code: append(append([]byte{1, 255}, bytes.Repeat([]byte{'c'}, 255)...), 0)}
	var many []Object
	for i := 0; i < 8; i++ {
		o := Object{Code: half.Code, Exports: map[string]int{}}
		if i == 0 {
			o.Exports["main"] = 0
		}
		many = append(many, o)
	}
	if _, err := Link(many); err != nil {
		t.Fatal("8 x 258 should link", err)
	}
	// 64 instructions pass, 65 fail.
	for _, c := range []struct {
		n  int
		ok bool
	}{{64, true}, {65, false}} {
		code := bytes.Repeat([]byte{1, 0}, c.n-1)
		code = append(code, 0)
		o := Object{Code: code, Exports: ex}
		if c.ok {
			if _, err := Link([]Object{o}); err != nil {
				t.Fatal("64 instructions rejected", err)
			}
		} else {
			mustFail(t, "65 instructions", []Object{o})
		}
	}
}
