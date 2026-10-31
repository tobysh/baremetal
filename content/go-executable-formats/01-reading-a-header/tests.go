package main

import (
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

func TestReadU32_bounds(t *testing.T) {
	b := []byte{1, 2, 3, 4, 5, 6}
	for off, want := range map[uint64]uint32{0: 0x04030201, 1: 0x05040302, 2: 0x06050403} {
		if v, err := ReadU32(b, off); err != nil || v != want {
			t.Errorf("ReadU32(off=%d) = %#x, %v; want %#x", off, v, err, want)
		}
	}
	for _, off := range []uint64{3, 4, 6, 7, 1 << 32, math.MaxUint64 - 1, math.MaxUint64} {
		if _, err := ReadU32(b, off); !errors.Is(err, ErrTruncated) {
			t.Errorf("ReadU32(off=%d) err = %v; want ErrTruncated", off, err)
		}
	}
	if _, err := ReadU32(nil, 0); !errors.Is(err, ErrTruncated) {
		t.Errorf("ReadU32 on nil: %v", err)
	}
}

func TestReadU16_bounds(t *testing.T) {
	b := []byte{0xAA, 0xBB, 0xCC}
	if v, err := ReadU16(b, 1); err != nil || v != 0xCCBB {
		t.Errorf("ReadU16(1) = %#x, %v; want 0xccbb", v, err)
	}
	for _, off := range []uint64{2, 3, 4, math.MaxUint64 - 1, math.MaxUint64} {
		if _, err := ReadU16(b, off); !errors.Is(err, ErrTruncated) {
			t.Errorf("ReadU16(off=%d) err = %v; want ErrTruncated", off, err)
		}
	}
}

func hdrSamples() []*tbObj {
	a, _ := tbPair(3, []uint32{1, 2, 3}, 8)
	return append(a, newProg().o, &tbObj{})
}

func TestParseHeader_valid(t *testing.T) {
	for n, o := range hdrSamples() {
		b := o.bytes()
		h, err := ParseHeader(b)
		if err != nil {
			t.Fatalf("object %d: %v", n, err)
		}
		want := Header{Version: 1, NSec: uint16(len(o.Secs)), NSym: uint16(len(o.Syms)), NRel: uint16(len(o.Rels)),
			StrSize: uint32(len(o.strtab())), SecOff: 16, SymOff: uint64(o.symAt(0)), RelOff: uint64(o.relAt(0)),
			StrOff: uint64(o.strOff()), End: uint64(o.tablesEnd())}
		if h != want {
			t.Errorf("object %d: header = %+v; want %+v", n, h, want)
		}
	}
}

func TestParseHeader_errors(t *testing.T) {
	good := hdrSamples()[0].bytes()
	mut := func(f func(b []byte)) []byte { b := append([]byte{}, good...); f(b); return b }
	cases := []struct {
		name string
		in   []byte
		want error
	}{
		{"nil", nil, ErrTruncated},
		{"short with bad magic", []byte("XXXX"), ErrTruncated},
		{"magic", mut(func(b []byte) { b[0] = 'X' }), ErrMagic},
		{"version 0", mut(func(b []byte) { b[4] = 0 }), ErrVersion},
		{"version 2", mut(func(b []byte) { b[4] = 2 }), ErrVersion},
		{"flags", mut(func(b []byte) { b[5] = 1 }), ErrCorrupt},
		{"huge string table", mut(func(b []byte) { tbPut32(b, 12, 0xFFFFFFFF) }), ErrTruncated},
		{"max section count", mut(func(b []byte) { tbPut16(b, 6, 0xFFFF) }), ErrTruncated},
		{"max everything", mut(func(b []byte) { tbPut16(b, 6, 0xFFFF); tbPut16(b, 8, 0xFFFF); tbPut16(b, 10, 0xFFFF); tbPut32(b, 12, 0xFFFFFFFF) }), ErrTruncated},
	}
	for _, c := range cases {
		if _, err := ParseHeader(c.in); !errors.Is(err, c.want) {
			t.Errorf("%s: err = %v; want %v", c.name, err, c.want)
		}
	}
	end := len(hdrSamples()[0].strtab()) + hdrSamples()[0].relAt(len(hdrSamples()[0].Rels))
	for n := 16; n < end; n++ {
		if _, err := ParseHeader(good[:n]); !errors.Is(err, ErrTruncated) {
			t.Fatalf("input cut to %d bytes (tables end at %d): err = %v; want ErrTruncated", n, end, err)
		}
	}
	if _, err := ParseHeader(good[:end]); err != nil {
		t.Errorf("input cut right after the string table is still a valid header, got %v", err)
	}
}
