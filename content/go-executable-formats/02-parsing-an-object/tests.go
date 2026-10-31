package main

import (
	"bytes"
	"encoding/binary"
	"errors"
	"fmt"
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

func sampleObjs() (lib, main []byte, lo, mo *tbObj) {
	os, _ := tbPair(4, []uint32{10, 20, 30}, 12)
	return os[1].bytes(), os[0].bytes(), os[1], os[0]
}

func checkInvariants(o *Object) string {
	for i, s := range o.Sections {
		if s.Kind == SecBss && s.Data != nil || s.Kind != SecBss && uint32(len(s.Data)) != s.Size {
			return fmt.Sprintf("section %d: size/data mismatch", i)
		}
	}
	for i, s := range o.Symbols {
		if s.Sec < -1 || s.Sec >= len(o.Sections) || (s.Sec >= 0 && s.Value > o.Sections[s.Sec].Size) {
			return fmt.Sprintf("symbol %d out of range", i)
		}
	}
	for i, r := range o.Relocs {
		if r.Sec < 0 || r.Sec >= len(o.Sections) || r.Sym < 0 || r.Sym >= len(o.Symbols) ||
			o.Sections[r.Sec].Kind == SecBss || uint64(r.Off)+4 > uint64(o.Sections[r.Sec].Size) {
			return fmt.Sprintf("reloc %d out of range", i)
		}
	}
	return ""
}

func TestParseObject_round_trip(t *testing.T) {
	for _, reps := range []uint32{1, 5} {
		for _, pad := range []int{0, 4, 20} {
			objs, _ := tbPair(reps, []uint32{1, 2, 3, 4}[:1+int(reps)%4], pad)
			for k, want := range objs {
				got, err := ParseObject(want.bytes())
				if err != nil {
					t.Fatalf("object %d: %v", k, err)
				}
				if len(got.Sections) != len(want.Secs) || len(got.Symbols) != len(want.Syms) || len(got.Relocs) != len(want.Rels) {
					t.Fatalf("object %d: counts %d/%d/%d; want %d/%d/%d", k, len(got.Sections), len(got.Symbols), len(got.Relocs), len(want.Secs), len(want.Syms), len(want.Rels))
				}
				for i, s := range want.Secs {
					g := got.Sections[i]
					size := uint32(len(s.Data))
					if s.Kind == SecBss {
						size = s.Size
					}
					if g.Name != s.Name || g.Kind != s.Kind || g.Size != size || !bytes.Equal(g.Data, s.Data) {
						t.Errorf("object %d section %d = %+v; want %+v", k, i, g, s)
					}
				}
				for i, s := range want.Syms {
					if g := got.Symbols[i]; g.Name != s.Name || g.Sec != s.Sec || g.Bind != s.Bind || g.Value != s.Val {
						t.Errorf("object %d symbol %d = %+v; want %+v", k, i, g, s)
					}
				}
				for i, r := range want.Rels {
					if g := got.Relocs[i]; g != (Reloc{r.Sec, r.Kind, r.Off, r.Sym, r.Addend}) {
						t.Errorf("object %d reloc %d = %+v; want %+v", k, i, g, r)
					}
				}
			}
		}
	}
}

func TestParseObject_negative_addend_and_empty(t *testing.T) {
	p := newProg()
	p.label("f", BindGlobal)
	p.liSym(0, "f", -8)
	o, err := ParseObject(p.o.bytes())
	if err != nil || o.Relocs[0].Addend != -8 || o.Sections[1].Size != 0 {
		t.Fatalf("got %+v, %v", o, err)
	}
	if o, err := ParseObject((&tbObj{}).bytes()); err != nil || len(o.Sections) != 0 {
		t.Errorf("an object with no tables must parse: %v", err)
	}
}

func TestParseObject_does_not_alias_input(t *testing.T) {
	_, mb, _, _ := sampleObjs()
	o, err := ParseObject(mb)
	if err != nil {
		t.Fatal(err)
	}
	before := append([]byte{}, o.Sections[0].Data...)
	for i := range mb {
		mb[i] = 0xEE
	}
	if !bytes.Equal(o.Sections[0].Data, before) {
		t.Errorf("section data changed when the input buffer was overwritten; copy it")
	}
}

func TestParseObject_rejects_each_rule(t *testing.T) {
	lb, mb, lo, mo := sampleObjs()
	type c struct {
		name string
		in   []byte
		off  int
		val  []byte
		want error
	}
	u32 := func(v uint32) []byte { return binary.LittleEndian.AppendUint32(nil, v) }
	u16 := func(v uint16) []byte { return binary.LittleEndian.AppendUint16(nil, v) }
	endL := lo.tablesEnd()
	cases := []c{
		{"section kind 0", lb, tbSecAt(0) + 4, []byte{0}, ErrCorrupt},
		{"section kind 4", lb, tbSecAt(0) + 4, []byte{4}, ErrCorrupt},
		{"section padding", lb, tbSecAt(1) + 6, []byte{1}, ErrCorrupt},
		{"section name outside strtab", lb, tbSecAt(0), u32(100000), ErrCorrupt},
		{"section name at length", lb, tbSecAt(0), u32(uint32(len(lo.strtab()))), ErrCorrupt},
		{"text contents inside tables", lb, tbSecAt(0) + 8, u32(0), ErrCorrupt},
		{"text contents just inside tables", lb, tbSecAt(0) + 8, u32(uint32(endL - 1)), ErrCorrupt},
		{"text contents offset wraps", lb, tbSecAt(0) + 8, u32(0xFFFFFFF0), ErrTruncated},
		{"text size past end", lb, tbSecAt(0) + 12, u32(0xFFFFFFFF), ErrTruncated},
		{"data size one past end", lb, tbSecAt(1) + 12, u32(uint32(len(lb)-endL) + 1), ErrTruncated},
		{"bss with file offset", lb, tbSecAt(2) + 8, u32(0x40), ErrCorrupt},
		{"bss too large", lb, tbSecAt(2) + 12, u32(1<<20 + 1), ErrCorrupt},
		{"symbol name outside strtab", lb, lo.symAt(0), u32(9999), ErrCorrupt},
		{"symbol section out of range", lb, lo.symAt(2) + 4, u16(3), ErrCorrupt},
		{"symbol bind 2", lb, lo.symAt(2) + 6, []byte{2}, ErrCorrupt},
		{"symbol padding", lb, lo.symAt(2) + 7, []byte{9}, ErrCorrupt},
		{"symbol value past section", lb, lo.symAt(2) + 8, u32(1000), ErrCorrupt},
		{"local undefined", mb, mo.symAt(0) + 4, u16(0xFFFF), ErrCorrupt},
		{"undefined with value", mb, mo.symAt(2) + 8, u32(4), ErrCorrupt},
		{"global without name", lb, lo.symAt(2), u32(0), ErrCorrupt},
		{"reloc kind", mb, mo.relAt(0) + 2, []byte{3}, ErrCorrupt},
		{"reloc padding", mb, mo.relAt(0) + 3, []byte{1}, ErrCorrupt},
		{"reloc reserved", mb, mo.relAt(0) + 10, u16(1), ErrCorrupt},
		{"reloc in bss", mb, mo.relAt(0), u16(2), ErrCorrupt},
		{"reloc section out of range", mb, mo.relAt(0), u16(9), ErrCorrupt},
		{"reloc field off the end", mb, mo.relAt(0) + 4, u32(uint32(len(mo.Secs[1].Data) - 3)), ErrCorrupt},
		{"reloc offset wraps", mb, mo.relAt(0) + 4, u32(0xFFFFFFFE), ErrCorrupt},
		{"reloc symbol out of range", mb, mo.relAt(0) + 8, u16(uint16(len(mo.Syms))), ErrCorrupt},
	}
	// terminator of the last name replaced: the name now runs off the table
	cases = append(cases, c{"unterminated name", lb, lo.strOff() + len(lo.strtab()) - 1, []byte{'x'}, ErrCorrupt})
	for _, tc := range cases {
		b := append([]byte{}, tc.in...)
		copy(b[tc.off:], tc.val)
		if _, err := ParseObject(b); !errors.Is(err, tc.want) {
			t.Errorf("%s: err = %v; want %v", tc.name, err, tc.want)
		}
	}
	// the valid boundary cases must still parse
	b := append([]byte{}, lb...)
	copy(b[lo.symAt(2)+8:], u32(4))
	if _, err := ParseObject(b); err != nil {
		t.Errorf("a symbol whose value equals the section size (an end marker) is valid: %v", err)
	}
	b = append([]byte{}, mb...)
	copy(b[mo.relAt(0)+4:], u32(uint32(len(mo.Secs[1].Data)-4)))
	if _, err := ParseObject(b); err != nil {
		t.Errorf("a relocation field that ends exactly at the section end is valid: %v", err)
	}
}

func TestParseObject_every_truncation_is_an_error(t *testing.T) {
	lb, mb, _, _ := sampleObjs()
	for _, full := range [][]byte{lb, mb} {
		for n := 0; n < len(full); n++ {
			if o, err := ParseObject(full[:n]); err == nil || o != nil {
				t.Fatalf("input cut to %d of %d bytes parsed without error", n, len(full))
			}
		}
	}
}

func TestParseObject_damaged_input_never_panics(t *testing.T) {
	lb, mb, _, _ := sampleObjs()
	for _, full := range [][]byte{lb, mb} {
		for i := range full {
			for _, v := range []byte{0x00, 0x01, 0x7F, 0x80, 0xFF} {
				b := append([]byte{}, full...)
				b[i] = v
				func() {
					defer func() {
						if r := recover(); r != nil {
							t.Fatalf("byte %d set to %#x: panic %v", i, v, r)
						}
					}()
					o, err := ParseObject(b)
					if err == nil {
						if msg := checkInvariants(o); msg != "" {
							t.Fatalf("byte %d set to %#x was accepted but %s", i, v, msg)
						}
					} else if !errors.Is(err, ErrTruncated) && !errors.Is(err, ErrCorrupt) && !errors.Is(err, ErrMagic) && !errors.Is(err, ErrVersion) {
						t.Fatalf("byte %d set to %#x: error %v wraps no Err value", i, v, err)
					}
				}()
			}
		}
	}
}
