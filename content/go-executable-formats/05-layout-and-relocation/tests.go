package main

import (
	"bytes"
	"encoding/binary"
	"errors"
	"math"
	"sort"
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

func parseAll(t *testing.T, os ...*tbObj) []*Object {
	t.Helper()
	var out []*Object
	for i, o := range os {
		p, err := ParseObject(o.bytes())
		if err != nil {
			t.Fatalf("object %d: %v", i, err)
		}
		out = append(out, p)
	}
	return out
}

func mkSec(kind uint8, n int) tbSec {
	if kind == SecBss {
		return tbSec{Name: "b", Kind: kind, Size: uint32(n)}
	}
	return tbSec{Name: "s", Kind: kind, Data: bytes.Repeat([]byte{0xAB}, n)}
}

func TestLayout_golden(t *testing.T) {
	// hand-computed: text 10 then 6 -> addresses 0x1000, 0x100c, TextSize 0x12;
	// data page: align(0x1012, 0x100) = 0x1100; data 5 then 3 -> 0x1100, 0x1108, size 11;
	// bss 8 and 1 start at align(11,4) = 12 -> 0x110c, 0x1114; MemSize 21.
	a := &tbObj{Secs: []tbSec{mkSec(SecData, 5), mkSec(SecText, 10), mkSec(SecBss, 8)}}
	b := &tbObj{Secs: []tbSec{mkSec(SecBss, 1), mkSec(SecText, 6), mkSec(SecData, 3)}}
	p, err := Layout(parseAll(t, a, b))
	if err != nil {
		t.Fatal(err)
	}
	if p.TextAddr != 0x1000 || p.TextSize != 0x12 || p.DataAddr != 0x1100 || p.DataSize != 11 || p.MemSize != 21 {
		t.Errorf("plan = %+v", p)
	}
	for _, c := range []struct {
		o, s int
		want uint32
	}{{0, 0, 0x1100}, {0, 1, 0x1000}, {0, 2, 0x110c}, {1, 0, 0x1114}, {1, 1, 0x100c}, {1, 2, 0x1108}} {
		if got := p.Addr(c.o, c.s); got != c.want {
			t.Errorf("Addr(%d,%d) = %#x; want %#x", c.o, c.s, got, c.want)
		}
	}
}

func TestLayout_edges(t *testing.T) {
	// no text at all: the data page starts at Base
	p, err := Layout(parseAll(t, &tbObj{Secs: []tbSec{mkSec(SecData, 4)}}))
	if err != nil || p.TextSize != 0 || p.DataAddr != 0x1000 || p.MemSize != 4 {
		t.Errorf("data only: %+v, %v", p, err)
	}
	// text ending exactly on a page boundary does not skip a page
	p, _ = Layout(parseAll(t, &tbObj{Secs: []tbSec{mkSec(SecText, 0x100), mkSec(SecData, 1)}}))
	if p.DataAddr != 0x1100 {
		t.Errorf("DataAddr = %#x; want 0x1100", p.DataAddr)
	}
	// no objects
	p, err = Layout(nil)
	if err != nil || p.TextSize != 0 || p.MemSize != 0 {
		t.Errorf("empty: %+v, %v", p, err)
	}
}

func TestLayout_random_against_the_model(t *testing.T) {
	r := tbRand(42)
	for round := 0; round < 60; round++ {
		var objs []*tbObj
		for i := r.n(4); i >= 0; i-- {
			o := &tbObj{}
			for j := r.n(6); j > 0; j-- {
				kind := []uint8{SecText, SecData, SecBss}[r.n(3)]
				o.Secs = append(o.Secs, mkSec(kind, r.n(70)))
			}
			objs = append(objs, o)
		}
		want := tbLayout(objs)
		got, err := Layout(parseAll(t, objs...))
		if err != nil {
			t.Fatalf("round %d: %v", round, err)
		}
		if got.TextSize != want.textSize || got.DataAddr != want.dataAddr || got.DataSize != want.dataSize || got.MemSize != want.memSize {
			t.Fatalf("round %d: plan %+v; want text %d data %#x/%d mem %d", round, got, want.textSize, want.dataAddr, want.dataSize, want.memSize)
		}
		for i := range objs {
			for j := range objs[i].Secs {
				if got.Addr(i, j) != want.addr[i][j] {
					t.Fatalf("round %d: Addr(%d,%d) = %#x; want %#x", round, i, j, got.Addr(i, j), want.addr[i][j])
				}
			}
		}
	}
}

func TestLayout_too_big(t *testing.T) {
	big := func(n int) *tbObj { return &tbObj{Secs: []tbSec{mkSec(SecBss, n)}} }
	if _, err := Layout(parseAll(t, big(1<<20))); !errors.Is(err, ErrTooBig) {
		t.Errorf("one maximal bss: %v", err)
	}
	if _, err := Layout(parseAll(t, big(600000), big(600000))); !errors.Is(err, ErrTooBig) {
		t.Errorf("two large bss: %v", err)
	}
	var many []*tbObj
	for i := 0; i < 20; i++ {
		many = append(many, &tbObj{Secs: []tbSec{mkSec(SecText, 60000)}})
	}
	if _, err := Layout(parseAll(t, many...)); !errors.Is(err, ErrTooBig) {
		t.Errorf("text overflow: %v", err)
	}
	// 0x1000 + 0xFF000 is exactly the limit and fits
	if _, err := Layout(parseAll(t, big(0xFF000))); err != nil {
		t.Errorf("an image ending exactly at the limit fits: %v", err)
	}
	if _, err := Layout(parseAll(t, big(0xFF001))); !errors.Is(err, ErrTooBig) {
		t.Errorf("one byte over the limit must fail")
	}
}

func linkParts(t *testing.T, os []*tbObj) ([]*Object, *Resolution, *Plan) {
	objs := parseAll(t, os...)
	res, err := Resolve(objs)
	if err != nil {
		t.Fatal(err)
	}
	p, err := Layout(objs)
	if err != nil {
		t.Fatal(err)
	}
	return objs, res, p
}

func TestRelocate_matches_the_reference_linker(t *testing.T) {
	r := tbRand(7)
	for round := 0; round < 12; round++ {
		table := make([]uint32, 1+r.n(5))
		for i := range table {
			table[i] = uint32(r.n(1000))
		}
		os, _ := tbPair(uint32(1+r.n(6)), table, 4*r.n(6))
		if round%2 == 1 {
			os[0], os[1] = os[1], os[0]
		}
		objs, res, plan := linkParts(t, os)
		img, err := Relocate(objs, res, plan)
		if err != nil {
			t.Fatal(err)
		}
		exe, _ := tbLink(os, "_start")
		nseg := int(exe[5])
		textOff := 12 + 20*nseg
		lay := tbLayout(os)
		if !bytes.Equal(img.Text, exe[textOff:textOff+int(lay.textSize)]) {
			t.Fatalf("round %d: text differs\n got % x\nwant % x", round, img.Text, exe[textOff:textOff+int(lay.textSize)])
		}
		if !bytes.Equal(img.Data, exe[textOff+int(lay.textSize):]) {
			t.Fatalf("round %d: data differs\n got % x\nwant % x", round, img.Data, exe[textOff+int(lay.textSize):])
		}
	}
}

func TestRelocate_ignores_old_field_bytes_and_uses_addends(t *testing.T) {
	p := newProg()
	p.label("_start", BindGlobal)
	p.liSym(0, "target", 6)   // abs32 with a positive addend
	p.liSym(1, "target", -2)  // abs32 with a negative addend
	p.call("target")          // forward rel32
	p.label("target", BindGlobal)
	p.jmp("_start")           // backward rel32
	for _, r := range p.o.Rels { // scribble over every field
		copy(p.o.Secs[0].Data[r.Off:], []byte{0xEE, 0xEE, 0xEE, 0xEE})
	}
	objs, res, plan := linkParts(t, []*tbObj{p.o})
	img, err := Relocate(objs, res, plan)
	if err != nil {
		t.Fatal(err)
	}
	target := uint32(0x1000 + 6 + 6 + 5)
	want := []uint32{target + 6, target - 2}
	for i, w := range want {
		if got := binary.LittleEndian.Uint32(img.Text[6*i+2:]); got != w {
			t.Errorf("abs32 %d = %#x; want %#x", i, got, w)
		}
	}
	if got := int32(binary.LittleEndian.Uint32(img.Text[13:])); got != 0 { // call: target is right after the call
		t.Errorf("call rel32 = %d; want 0", got)
	}
	if got := int32(binary.LittleEndian.Uint32(img.Text[18:])); got != -(18 + 4) { // jmp back to 0x1000
		t.Errorf("jmp rel32 = %d; want %d", got, -(18 + 4))
	}
}

func TestRelocate_range_errors(t *testing.T) {
	check := func(name string, mk func(p *tbProg), want error) {
		p := newProg()
		p.label("_start", BindGlobal)
		mk(p)
		p.emit(0)
		p.label("end", BindGlobal)
		p.halt()
		objs, res, plan := linkParts(t, []*tbObj{p.o})
		_, err := Relocate(objs, res, plan)
		if want == nil && err != nil || want != nil && !errors.Is(err, want) {
			t.Errorf("%s: err = %v; want %v", name, err, want)
		}
	}
	check("abs32 below zero", func(p *tbProg) { p.liSym(0, "_start", -0x1001) }, ErrRelocRange)
	check("abs32 exactly zero", func(p *tbProg) { p.liSym(0, "_start", -0x1000) }, nil)
	check("abs32 large positive addend", func(p *tbProg) { p.liSym(0, "_start", math.MaxInt32) }, nil)
	check("rel32 forward overflow", func(p *tbProg) { p.emit(10); p.field(RelRel32, "end", math.MaxInt32) }, ErrRelocRange)
	check("rel32 backward overflow", func(p *tbProg) { p.emit(10); p.field(RelRel32, "_start", math.MinInt32) }, ErrRelocRange)
	check("rel32 in range", func(p *tbProg) { p.emit(10); p.field(RelRel32, "end", 0) }, nil)
}

func TestRelocate_does_not_modify_its_inputs(t *testing.T) {
	os, _ := tbPair(2, []uint32{1, 2}, 4)
	objs, res, plan := linkParts(t, os)
	before := append([]byte{}, objs[0].Sections[0].Data...)
	if _, err := Relocate(objs, res, plan); err != nil {
		t.Fatal(err)
	}
	if !bytes.Equal(before, objs[0].Sections[0].Data) {
		t.Errorf("Relocate patched the object's own section data; patch the image copy")
	}
}
