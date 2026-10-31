package main

import (
	"bytes"
	"encoding/binary"
	"errors"
	"fmt"
	"reflect"
	"sort"
	"strings"
	"testing"
)

// ---- test support: an independent object builder, assembler and reference linker ----

type tbSec struct {
	Name string
	Kind uint8
	Data []byte
	Size uint32
}
type tbSym struct {
	Name string
	Sec  int
	Bind uint8
	Val  uint32
}
type tbRel struct {
	Sec    int
	Kind   uint8
	Off    uint32
	Sym    int
	Addend int32
}
type tbObj struct {
	Secs []tbSec
	Syms []tbSym
	Rels []tbRel
}

func tbPut32(b []byte, off int, v uint32) { binary.LittleEndian.PutUint32(b[off:], v) }
func tbPut16(b []byte, off int, v uint16) { binary.LittleEndian.PutUint16(b[off:], v) }

// Table positions of a built object.
func tbSecAt(i int) int            { return 16 + 16*i }
func (o *tbObj) symAt(i int) int   { return 16 + 16*len(o.Secs) + 12*i }
func (o *tbObj) relAt(i int) int   { return 16 + 16*len(o.Secs) + 12*len(o.Syms) + 16*i }
func (o *tbObj) tablesEnd() int    { return o.relAt(len(o.Rels)) + len(o.strtab()) }
func (o *tbObj) strtab() []byte    { s, _ := o.strings(); return s }
func (o *tbObj) strOff() int       { return o.relAt(len(o.Rels)) }
func (o *tbObj) nameOffs() map[string]uint32 { _, m := o.strings(); return m }

func (o *tbObj) strings() ([]byte, map[string]uint32) {
	str := []byte{0}
	m := map[string]uint32{"": 0}
	add := func(s string) {
		if _, ok := m[s]; !ok {
			m[s] = uint32(len(str))
			str = append(append(str, s...), 0)
		}
	}
	for _, s := range o.Secs {
		add(s.Name)
	}
	for _, s := range o.Syms {
		add(s.Name)
	}
	return str, m
}

func (o *tbObj) bytes() []byte {
	str, m := o.strings()
	b := make([]byte, o.relAt(len(o.Rels))+len(str))
	copy(b, "TOBJ")
	b[4] = 1
	tbPut16(b, 6, uint16(len(o.Secs)))
	tbPut16(b, 8, uint16(len(o.Syms)))
	tbPut16(b, 10, uint16(len(o.Rels)))
	tbPut32(b, 12, uint32(len(str)))
	copy(b[o.strOff():], str)
	for i, s := range o.Secs {
		p := tbSecAt(i)
		tbPut32(b, p, m[s.Name])
		b[p+4] = s.Kind
		if s.Kind == SecBss {
			tbPut32(b, p+12, s.Size)
			continue
		}
		tbPut32(b, p+8, uint32(len(b)))
		tbPut32(b, p+12, uint32(len(s.Data)))
		b = append(b, s.Data...)
	}
	for i, s := range o.Syms {
		p := o.symAt(i)
		tbPut32(b, p, m[s.Name])
		sec := uint16(0xFFFF)
		if s.Sec >= 0 {
			sec = uint16(s.Sec)
		}
		tbPut16(b, p+4, sec)
		b[p+6] = s.Bind
		tbPut32(b, p+8, s.Val)
	}
	for i, r := range o.Rels {
		p := o.relAt(i)
		tbPut16(b, p, uint16(r.Sec))
		b[p+2] = r.Kind
		tbPut32(b, p+4, r.Off)
		tbPut16(b, p+8, uint16(r.Sym))
		tbPut32(b, p+12, uint32(r.Addend))
	}
	return b
}

// tbProg assembles one object with sections 0=text, 1=data, 2=bss.
type tbProg struct{ o *tbObj }

func newProg() *tbProg {
	return &tbProg{&tbObj{Secs: []tbSec{{Name: ".text", Kind: SecText}, {Name: ".data", Kind: SecData}, {Name: ".bss", Kind: SecBss}}}}
}
func (p *tbProg) sym(name string) int {
	for i, s := range p.o.Syms {
		if s.Name == name {
			return i
		}
	}
	p.o.Syms = append(p.o.Syms, tbSym{Name: name, Sec: -1, Bind: BindGlobal})
	return len(p.o.Syms) - 1
}
func (p *tbProg) def(name string, sec int, bind uint8, val uint32) {
	for i, s := range p.o.Syms {
		if s.Name == name && s.Sec < 0 {
			p.o.Syms[i] = tbSym{name, sec, bind, val}
			return
		}
	}
	p.o.Syms = append(p.o.Syms, tbSym{name, sec, bind, val})
}
func (p *tbProg) text() *tbSec { return &p.o.Secs[0] }
func (p *tbProg) label(name string, bind uint8) {
	p.def(name, 0, bind, uint32(len(p.o.Secs[0].Data)))
}
func (p *tbProg) word(name string, bind uint8, vals ...uint32) {
	if name != "" {
		p.def(name, 1, bind, uint32(len(p.o.Secs[1].Data)))
	}
	for _, v := range vals {
		p.o.Secs[1].Data = binary.LittleEndian.AppendUint32(p.o.Secs[1].Data, v)
	}
}
func (p *tbProg) ptr(name string, bind uint8, target string, addend int32) {
	p.def(name, 1, bind, uint32(len(p.o.Secs[1].Data)))
	p.o.Rels = append(p.o.Rels, tbRel{1, RelAbs32, uint32(len(p.o.Secs[1].Data)), p.sym(target), addend})
	p.o.Secs[1].Data = append(p.o.Secs[1].Data, 0, 0, 0, 0)
}
func (p *tbProg) bss(name string, bind uint8, size uint32) {
	p.def(name, 2, bind, p.o.Secs[2].Size)
	p.o.Secs[2].Size += size
}
func (p *tbProg) emit(b ...byte) { p.o.Secs[0].Data = append(p.o.Secs[0].Data, b...) }
func (p *tbProg) field(kind uint8, target string, addend int32) {
	p.o.Rels = append(p.o.Rels, tbRel{0, kind, uint32(len(p.o.Secs[0].Data)), p.sym(target), addend})
	p.emit(0, 0, 0, 0)
}
func (p *tbProg) imm(v uint32) { p.emit(byte(v), byte(v>>8), byte(v>>16), byte(v>>24)) }
func (p *tbProg) li(r byte, v uint32)           { p.emit(1, r); p.imm(v) }
func (p *tbProg) liSym(r byte, s string, a int32) { p.emit(1, r); p.field(RelAbs32, s, a) }
func (p *tbProg) load(r byte, s string, a int32)  { p.emit(2, r); p.field(RelAbs32, s, a) }
func (p *tbProg) store(r byte, s string, a int32) { p.emit(3, r); p.field(RelAbs32, s, a) }
func (p *tbProg) add(a, b byte)                   { p.emit(4, a, b) }
func (p *tbProg) sub(a, b byte)                   { p.emit(5, a, b) }
func (p *tbProg) call(s string)                   { p.emit(6); p.field(RelRel32, s, 0) }
func (p *tbProg) ret()                            { p.emit(7) }
func (p *tbProg) jnz(r byte, s string)            { p.emit(8, r); p.field(RelRel32, s, 0) }
func (p *tbProg) jmp(s string)                    { p.emit(10); p.field(RelRel32, s, 0) }
func (p *tbProg) out(r byte)                      { p.emit(9, r) }
func (p *tbProg) halt()                           { p.emit(0) }

func tbAlign(v, a uint64) uint64 { return (v + a - 1) / a * a }

// tbLayout is the reference layout model, written from the rules in the task.
type tbLay struct {
	addr                                  [][]uint32
	textSize, dataAddr, dataSize, memSize uint32
}

func tbLayout(objs []*tbObj) tbLay {
	l := tbLay{addr: make([][]uint32, len(objs))}
	for i, o := range objs {
		l.addr[i] = make([]uint32, len(o.Secs))
	}
	size := func(s tbSec) uint64 {
		if s.Kind == SecBss {
			return uint64(s.Size)
		}
		return uint64(len(s.Data))
	}
	var cur uint64
	for _, kind := range []uint8{SecText, SecData, SecBss} {
		if kind == SecData {
			l.textSize = uint32(cur)
			l.dataAddr = uint32(tbAlign(0x1000+cur, 0x100))
			cur = 0
		}
		if kind == SecBss {
			l.dataSize = uint32(cur)
		}
		for i, o := range objs {
			for j, s := range o.Secs {
				if s.Kind == kind {
					cur = tbAlign(cur, 4)
					base := uint64(0x1000)
					if kind != SecText {
						base = uint64(l.dataAddr)
					}
					l.addr[i][j] = uint32(base + cur)
					cur += size(s)
				}
			}
		}
	}
	l.memSize = uint32(cur)
	return l
}

func (l tbLay) symAddr(objs []*tbObj, name string) (uint32, bool) {
	for i, o := range objs {
		for _, s := range o.Syms {
			if s.Name == name && s.Bind == BindGlobal && s.Sec >= 0 {
				return l.addr[i][s.Sec] + s.Val, true
			}
		}
	}
	return 0, false
}

// tbLink links objects (assumed valid) and returns the executable bytes and the link map.
func tbLink(objs []*tbObj, entry string) ([]byte, []tbMap) {
	l := tbLayout(objs)
	text := make([]byte, l.textSize)
	data := make([]byte, l.dataSize)
	seg := func(s tbSec, i, j int) []byte {
		if s.Kind == SecText {
			return text[l.addr[i][j]-0x1000:]
		}
		return data[l.addr[i][j]-l.dataAddr:]
	}
	for i, o := range objs {
		for j, s := range o.Secs {
			if s.Kind != SecBss {
				copy(seg(s, i, j), s.Data)
			}
		}
	}
	for i, o := range objs {
		for _, r := range o.Rels {
			sy := o.Syms[r.Sym]
			var S uint32
			if sy.Sec >= 0 {
				S = l.addr[i][sy.Sec] + sy.Val
			} else {
				S, _ = l.symAddr(objs, sy.Name)
			}
			P := l.addr[i][r.Sec] + r.Off
			v := S + uint32(r.Addend)
			if r.Kind == RelRel32 {
				v -= P + 4
			}
			tbPut32(seg(o.Secs[r.Sec], i, r.Sec), int(r.Off), v)
		}
	}
	ent, _ := l.symAddr(objs, entry)
	var exe []byte
	nseg := 1
	if l.memSize > 0 {
		nseg = 2
	}
	exe = append([]byte("TEXE"), 1, byte(nseg), 0, 0)
	u32 := func(vs ...uint32) {
		for _, v := range vs {
			exe = binary.LittleEndian.AppendUint32(exe, v)
		}
	}
	u32(ent)
	off := uint32(12 + 20*nseg)
	u32(5, 0x1000, off, l.textSize, l.textSize)
	if nseg == 2 {
		u32(6, l.dataAddr, off+l.textSize, l.dataSize, l.memSize)
	}
	exe = append(append(exe, text...), data...)
	var m []tbMap
	for i, o := range objs {
		for _, s := range o.Syms {
			if s.Sec >= 0 && s.Bind == BindGlobal {
				m = append(m, tbMap{s.Name, l.addr[i][s.Sec] + s.Val})
			}
		}
	}
	sort.Slice(m, func(a, b int) bool {
		if m[a].Addr != m[b].Addr {
			return m[a].Addr < m[b].Addr
		}
		return m[a].Name < m[b].Name
	})
	return exe, m
}

type tbMap struct {
	Name string
	Addr uint32
}

// tbRand is a tiny deterministic generator so the cases never change between runs.
type tbRand uint64

func (r *tbRand) n(k int) int {
	*r = *r*6364136223846793005 + 1442695040888963407
	return int((uint64(*r) >> 33) % uint64(k))
}

// tbPair builds the two-object program used by several lessons: lib.o holds a
// counter in bss, a table in data and a bump function; main.o loops `reps`
// times calling bump, then adds up the table and prints the total, the
// address of the counter (through a data pointer) and halts.
// It returns the objects and the output a correct link must produce.
func tbPair(reps uint32, table []uint32, pad int) ([]*tbObj, []uint32) {
	lib := newProg()
	lib.bss("pad_bss", BindLocal, uint32(pad))
	lib.word("pad_data", BindLocal, make([]uint32, pad/4)...)
	lib.bss("counter", BindGlobal, 4)
	lib.word("table", BindGlobal, table...)
	lib.label("bump", BindGlobal)
	lib.load(2, "counter", 0)
	lib.li(3, 1)
	lib.add(2, 3)
	lib.store(2, "counter", 0)
	lib.out(2)
	lib.ret()

	m := newProg()
	m.word("reps", BindLocal, reps)
	m.ptr("counter_ptr", BindLocal, "counter", 0)
	m.label("_start", BindGlobal)
	m.load(1, "reps", 0)
	m.label("loop", BindLocal)
	m.call("bump")
	m.li(3, 1)
	m.sub(1, 3)
	m.jnz(1, "loop")
	m.li(0, 0)
	for i := range table {
		m.load(2, "table", int32(4*i))
		m.add(0, 2)
	}
	m.out(0)
	m.load(0, "counter_ptr", 0)
	m.out(0)
	m.halt()

	var want []uint32
	for i := uint32(1); i <= reps; i++ {
		want = append(want, i)
	}
	var sum uint32
	for _, v := range table {
		sum += v
	}
	want = append(want, sum)
	return []*tbObj{m.o, lib.o}, want
}

func counterAddr(os []*tbObj) uint32 {
	a, _ := tbLayout(os).symAddr(os, "counter")
	return a
}

func TestLink_matches_the_reference_for_many_programs_and_orders(t *testing.T) {
	r := tbRand(2024)
	for round := 0; round < 16; round++ {
		table := make([]uint32, 1+r.n(6))
		for i := range table {
			table[i] = uint32(r.n(100000))
		}
		reps := uint32(1 + r.n(7))
		os, want := tbPair(reps, table, 4*r.n(8))
		if round%2 == 1 {
			os[0], os[1] = os[1], os[0]
		}
		want = append(want, counterAddr(os))
		var raw [][]byte
		for _, o := range os {
			raw = append(raw, o.bytes())
		}
		keep := [][]byte{append([]byte{}, raw[0]...), append([]byte{}, raw[1]...)}
		ref, refMap0 := tbLink(os, "_start")
		var refMap []MapEntry
		for _, m := range refMap0 {
			refMap = append(refMap, MapEntry(m))
		}
		l, err := Link(raw, "_start")
		if err != nil {
			t.Fatalf("round %d: %v", round, err)
		}
		if !bytes.Equal(l.Exe, ref) {
			t.Fatalf("round %d: executable differs from the reference\n got % x\nwant % x", round, l.Exe, ref)
		}
		if !reflect.DeepEqual(l.Map, refMap) {
			t.Fatalf("round %d: map = %v; want %v", round, l.Map, refMap)
		}
		if !bytes.Equal(raw[0], keep[0]) || !bytes.Equal(raw[1], keep[1]) {
			t.Fatalf("round %d: Link modified its input", round)
		}
		run, err := LinkAndRun(raw, "_start", 10000)
		if err != nil {
			t.Fatalf("round %d: run: %v", round, err)
		}
		if !reflect.DeepEqual(run.Output, want) {
			t.Fatalf("round %d: output %v; want %v", round, run.Output, want)
		}
		parsed, _ := ParseExe(ref)
		if run.Description != Describe(parsed) || !strings.HasPrefix(run.Description, "TEXE entry=") {
			t.Fatalf("round %d: description %q", round, run.Description)
		}
		if !bytes.Equal(run.Exe, ref) || !reflect.DeepEqual(run.Map, refMap) {
			t.Fatalf("round %d: Run must carry the Linked executable and map", round)
		}
	}
}

func TestLink_map_is_sorted_and_global_only(t *testing.T) {
	os, _ := tbPair(2, []uint32{1, 2}, 4)
	l, err := Link([][]byte{os[0].bytes(), os[1].bytes()}, "_start")
	if err != nil {
		t.Fatal(err)
	}
	var names []string
	for i, e := range l.Map {
		names = append(names, e.Name)
		if i > 0 && l.Map[i-1].Addr > e.Addr {
			t.Errorf("map not sorted by address: %v", l.Map)
		}
	}
	if !reflect.DeepEqual(names[:1], []string{"_start"}) || len(names) != 4 {
		t.Errorf("map names = %v; want _start first and 4 globals (_start, bump, counter, table)", names)
	}
}

func raw(os ...*tbObj) [][]byte {
	var out [][]byte
	for _, o := range os {
		out = append(out, o.bytes())
	}
	return out
}

func TestLink_failures_name_their_cause(t *testing.T) {
	os, _ := tbPair(2, []uint32{1, 2}, 4)
	good := raw(os...)
	trunc := append([]byte{}, good[1][:len(good[1])-1]...)
	dup := newProg()
	dup.label("bump", BindGlobal)
	dup.ret()
	over := newProg()
	over.label("_start", BindGlobal)
	over.liSym(0, "_start", -0x2000)
	over.halt()
	huge := newProg()
	huge.bss("a", BindLocal, 1<<20)
	huge.label("_start", BindGlobal)
	huge.halt()
	cases := []struct {
		name  string
		in    [][]byte
		entry string
		want  error
		text  string
	}{
		{"second object truncated", [][]byte{good[0], trunc}, "_start", ErrTruncated, "object 1"},
		{"first object has bad magic", [][]byte{append([]byte("XXXX"), good[0][4:]...), good[1]}, "_start", ErrMagic, "object 0"},
		{"duplicate", append(raw(os...), dup.o.bytes()), "_start", ErrDuplicate, "bump"},
		{"undefined", good[:1], "_start", ErrUndefined, "counter"},
		{"relocation range", raw(over.o), "_start", ErrRelocRange, ""},
		{"too big", raw(huge.o), "_start", ErrTooBig, ""},
		{"missing entry", good, "main", ErrEntry, "main"},
		{"entry is data", good, "table", ErrEntry, "table"},
		{"entry is bss", good, "counter", ErrEntry, ""},
		{"no objects", nil, "_start", ErrEntry, ""},
	}
	for _, c := range cases {
		l, err := Link(c.in, c.entry)
		if !errors.Is(err, c.want) || l != nil || !strings.Contains(fmt.Sprint(err), c.text) {
			t.Errorf("%s: Link = %v, %v; want nil and %v mentioning %q", c.name, l, err, c.want, c.text)
		}
		if run, err := LinkAndRun(c.in, c.entry, 100); run != nil || !errors.Is(err, c.want) {
			t.Errorf("%s: LinkAndRun = %v, %v; want nil and %v", c.name, run, err, c.want)
		}
	}
}

func TestLinkAndRun_runtime_failures_keep_the_output(t *testing.T) {
	// writes into its own text segment: the machine must fault, after the first OUT
	w := newProg()
	w.label("_start", BindGlobal)
	w.li(0, 77)
	w.out(0)
	w.store(0, "_start", 0)
	w.out(0)
	w.halt()
	run, err := LinkAndRun(raw(w.o), "_start", 100)
	if !errors.Is(err, ErrFault) || run == nil || !reflect.DeepEqual(run.Output, []uint32{77}) || run.Description == "" {
		t.Errorf("store to text: run = %+v, err = %v; want ErrFault with output [77] and a description", run, err)
	}
	// an endless loop must stop at the step limit
	l := newProg()
	l.label("_start", BindGlobal)
	l.li(0, 5)
	l.label("again", BindLocal)
	l.out(0)
	l.jmp("again")
	run, err = LinkAndRun(raw(l.o), "_start", 31)
	if !errors.Is(err, ErrSteps) || run == nil || len(run.Output) != 15 {
		t.Errorf("endless loop: err = %v, run = %+v; want ErrSteps with 15 outputs (31 steps = LI + 15 OUT/JMP pairs)", err, run)
	}
	// 3 steps are enough for LI, OUT, HALT
	h := newProg()
	h.label("_start", BindGlobal)
	h.li(0, 9)
	h.out(0)
	h.halt()
	if run, err := LinkAndRun(raw(h.o), "_start", 3); err != nil || !reflect.DeepEqual(run.Output, []uint32{9}) {
		t.Errorf("exact step budget: %+v, %v", run, err)
	}
}

func TestEmit_shapes(t *testing.T) {
	text := []byte{0, 0, 0, 0}
	// text only: one segment
	img := &Image{Plan: &Plan{TextAddr: 0x1000, TextSize: 4, DataAddr: 0x1100}, Text: text}
	b, err := Emit(img, 0x1003)
	want := append([]byte("TEXE\x01\x01\x00\x00\x03\x10\x00\x00"), 5, 0, 0, 0, 0, 0x10, 0, 0, 32, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0)
	want = append(want, text...)
	if err != nil || !bytes.Equal(b, want) {
		t.Errorf("text-only image:\n got % x (%v)\nwant % x", b, err, want)
	}
	// bss only data segment still gets a segment, with filesz 0
	img = &Image{Plan: &Plan{TextAddr: 0x1000, TextSize: 4, DataAddr: 0x1100, MemSize: 16}, Text: text}
	b, err = Emit(img, 0x1000)
	if err != nil || len(b) != 12+40+4 || b[5] != 2 {
		t.Fatalf("bss-only: %v len %d", err, len(b))
	}
	e, err := ParseExe(b)
	if err != nil || e.Segs[1].FileSize != 0 || e.Segs[1].MemSize != 16 || e.Segs[1].Flags != 6 {
		t.Errorf("bss-only segment = %+v, %v", e, err)
	}
	for _, ent := range []uint32{0x0fff, 0x1004, 0x1100, 0} {
		if _, err := Emit(img, ent); !errors.Is(err, ErrEntry) {
			t.Errorf("entry %#x: err = %v; want ErrEntry", ent, err)
		}
	}
	if _, err := Emit(&Image{Plan: &Plan{TextAddr: 0x1000}}, 0x1000); !errors.Is(err, ErrEntry) {
		t.Errorf("an image with no text has no valid entry: %v", err)
	}
}
