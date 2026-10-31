package main

import (
	"bytes"
	"encoding/binary"
	"errors"
	"fmt"
	"math"
	"strings"
)

// ---- TOBJ: the invented object format ------------------------------------
//
// All integers are little-endian.
//
//   header (16 bytes): "TOBJ", version u8 (=1), flags u8 (=0), nsec u16,
//                      nsym u16, nrel u16, strsize u32
//   then, back to back: nsec section entries (16 bytes), nsym symbol
//   entries (12 bytes), nrel relocation entries (16 bytes), strsize bytes of
//   string table. Section contents follow somewhere after that.
//
//   section: nameoff u32, kind u8, pad[3], offset u32, size u32
//   symbol:  nameoff u32, section u16 (0xFFFF = undefined), bind u8, pad u8, value u32
//   reloc:   section u16, kind u8, pad u8, offset u32, symbol u16, reserved u16, addend i32

const objMagic = "TOBJ"

const (
	SecText uint8 = 1
	SecData uint8 = 2
	SecBss  uint8 = 3
)

const (
	BindLocal  uint8 = 0
	BindGlobal uint8 = 1
)

const (
	RelAbs32 uint8 = 1 // field = S + A
	RelRel32 uint8 = 2 // field = S + A - (P + 4), P = address of the field
)

const (
	noSection = 0xFFFF
	maxBss    = 1 << 20
)

type Header struct {
	Version, Flags uint8
	NSec, NSym     uint16
	NRel           uint16
	StrSize        uint32
	// Byte offsets of each table; End is the first byte after the string table.
	SecOff, SymOff, RelOff, StrOff, End uint64
}

type Section struct {
	Name string
	Kind uint8
	Data []byte // nil for bss
	Size uint32 // len(Data) for text and data
}

type Symbol struct {
	Name  string
	Sec   int // index into Object.Sections, or -1 when undefined
	Bind  uint8
	Value uint32 // offset inside the section
}

type Reloc struct {
	Sec    int
	Kind   uint8
	Off    uint32
	Sym    int
	Addend int32
}

type Object struct {
	Sections []Section
	Symbols  []Symbol
	Relocs   []Reloc
}

func le16(b []byte) uint16 { return binary.LittleEndian.Uint16(b) }
func le32(b []byte) uint32 { return binary.LittleEndian.Uint32(b) }

// Every failure wraps exactly one of these, so callers can use errors.Is.
var (
	ErrTruncated = errors.New("truncated input")
	ErrMagic     = errors.New("bad magic")
	ErrVersion   = errors.New("unsupported version")
	ErrCorrupt   = errors.New("corrupt structure")
)

func ReadU16(b []byte, off uint64) (uint16, error) {
	if off > uint64(len(b)) || uint64(len(b))-off < 2 {
		return 0, fmt.Errorf("read u16 at %d of %d bytes: %w", off, len(b), ErrTruncated)
	}
	return binary.LittleEndian.Uint16(b[off:]), nil
}

func ReadU32(b []byte, off uint64) (uint32, error) {
	if off > uint64(len(b)) || uint64(len(b))-off < 4 {
		return 0, fmt.Errorf("read u32 at %d of %d bytes: %w", off, len(b), ErrTruncated)
	}
	return binary.LittleEndian.Uint32(b[off:]), nil
}

func ParseHeader(b []byte) (Header, error) {
	var h Header
	if len(b) < 16 {
		return h, fmt.Errorf("header needs 16 bytes, have %d: %w", len(b), ErrTruncated)
	}
	if string(b[0:4]) != objMagic {
		return h, fmt.Errorf("magic %q: %w", b[0:4], ErrMagic)
	}
	if b[4] != 1 {
		return h, fmt.Errorf("version %d: %w", b[4], ErrVersion)
	}
	if b[5] != 0 {
		return h, fmt.Errorf("flags %#x must be zero: %w", b[5], ErrCorrupt)
	}
	h.Version, h.Flags = b[4], b[5]
	h.NSec, h.NSym, h.NRel = le16(b[6:]), le16(b[8:]), le16(b[10:])
	h.StrSize = le32(b[12:])
	h.SecOff = 16
	h.SymOff = h.SecOff + 16*uint64(h.NSec)
	h.RelOff = h.SymOff + 12*uint64(h.NSym)
	h.StrOff = h.RelOff + 16*uint64(h.NRel)
	h.End = h.StrOff + uint64(h.StrSize)
	if h.End > uint64(len(b)) {
		return h, fmt.Errorf("tables end at %d but input is %d bytes: %w", h.End, len(b), ErrTruncated)
	}
	return h, nil
}

func ParseObject(b []byte) (*Object, error) {
	h, err := ParseHeader(b)
	if err != nil {
		return nil, err
	}
	str := b[h.StrOff:h.End]
	name := func(off uint32) (string, error) {
		if uint64(off) >= uint64(len(str)) {
			return "", fmt.Errorf("name offset %d outside the %d-byte string table: %w", off, len(str), ErrCorrupt)
		}
		i := bytes.IndexByte(str[off:], 0)
		if i < 0 {
			return "", fmt.Errorf("name at %d has no terminator: %w", off, ErrCorrupt)
		}
		return string(str[off : int(off)+i]), nil
	}
	o := &Object{}
	for i := 0; i < int(h.NSec); i++ {
		e := b[h.SecOff+uint64(i)*16:][:16]
		nm, err := name(le32(e[0:]))
		if err != nil {
			return nil, err
		}
		kind := e[4]
		if kind < SecText || kind > SecBss || e[5]|e[6]|e[7] != 0 {
			return nil, fmt.Errorf("section %d: bad kind or padding: %w", i, ErrCorrupt)
		}
		off, size := le32(e[8:]), le32(e[12:])
		s := Section{Name: nm, Kind: kind, Size: size}
		if kind == SecBss {
			if off != 0 || size > maxBss {
				return nil, fmt.Errorf("section %d: bad bss: %w", i, ErrCorrupt)
			}
		} else {
			if uint64(off) < h.End {
				return nil, fmt.Errorf("section %d: contents overlap the tables: %w", i, ErrCorrupt)
			}
			if uint64(off)+uint64(size) > uint64(len(b)) {
				return nil, fmt.Errorf("section %d: contents past end of file: %w", i, ErrTruncated)
			}
			s.Data = append([]byte{}, b[off:uint64(off)+uint64(size)]...)
		}
		o.Sections = append(o.Sections, s)
	}
	for i := 0; i < int(h.NSym); i++ {
		e := b[h.SymOff+uint64(i)*12:][:12]
		nm, err := name(le32(e[0:]))
		if err != nil {
			return nil, err
		}
		sec, bind, val := le16(e[4:]), e[6], le32(e[8:])
		if bind > BindGlobal || e[7] != 0 {
			return nil, fmt.Errorf("symbol %d: bad bind or padding: %w", i, ErrCorrupt)
		}
		s := Symbol{Name: nm, Bind: bind, Value: val, Sec: int(sec)}
		if sec == noSection {
			if bind != BindGlobal || val != 0 {
				return nil, fmt.Errorf("symbol %d: undefined symbols must be global with value 0: %w", i, ErrCorrupt)
			}
			s.Sec = -1
		} else {
			if int(sec) >= len(o.Sections) || val > o.Sections[sec].Size {
				return nil, fmt.Errorf("symbol %d: section or value out of range: %w", i, ErrCorrupt)
			}
		}
		if bind == BindGlobal && nm == "" {
			return nil, fmt.Errorf("symbol %d: global symbol with empty name: %w", i, ErrCorrupt)
		}
		o.Symbols = append(o.Symbols, s)
	}
	for i := 0; i < int(h.NRel); i++ {
		e := b[h.RelOff+uint64(i)*16:][:16]
		sec, kind, off, sym := int(le16(e[0:])), e[2], le32(e[4:]), int(le16(e[8:]))
		if e[3] != 0 || le16(e[10:]) != 0 || (kind != RelAbs32 && kind != RelRel32) {
			return nil, fmt.Errorf("reloc %d: bad kind or padding: %w", i, ErrCorrupt)
		}
		if sec >= len(o.Sections) || o.Sections[sec].Kind == SecBss || uint64(off)+4 > uint64(o.Sections[sec].Size) {
			return nil, fmt.Errorf("reloc %d: field is not inside a text or data section: %w", i, ErrCorrupt)
		}
		if sym >= len(o.Symbols) {
			return nil, fmt.Errorf("reloc %d: symbol %d out of range: %w", i, sym, ErrCorrupt)
		}
		o.Relocs = append(o.Relocs, Reloc{Sec: sec, Kind: kind, Off: off, Sym: sym, Addend: int32(le32(e[12:]))})
	}
	return o, nil
}

var (
	ErrDuplicate = errors.New("duplicate symbol")
	ErrUndefined = errors.New("undefined symbol")
)

// Def says where a symbol lives: section Sec of object Obj, Off bytes in.
type Def struct {
	Obj, Sec int
	Off      uint32
}

type Resolution struct {
	objs    []*Object
	globals map[string]Def
}

func Resolve(objs []*Object) (*Resolution, error) {
	r := &Resolution{objs: objs, globals: map[string]Def{}}
	for i, o := range objs {
		for _, s := range o.Symbols {
			if s.Sec < 0 || s.Bind != BindGlobal {
				continue
			}
			if d, dup := r.globals[s.Name]; dup {
				return nil, fmt.Errorf("symbol %q defined in objects %d and %d: %w", s.Name, d.Obj, i, ErrDuplicate)
			}
			r.globals[s.Name] = Def{Obj: i, Sec: s.Sec, Off: s.Value}
		}
	}
	for i, o := range objs {
		for _, s := range o.Symbols {
			if s.Sec < 0 {
				if _, ok := r.globals[s.Name]; !ok {
					return nil, fmt.Errorf("symbol %q (needed by object %d): %w", s.Name, i, ErrUndefined)
				}
			}
		}
	}
	return r, nil
}

func (r *Resolution) Lookup(name string) (Def, bool) {
	d, ok := r.globals[name]
	return d, ok
}

func (r *Resolution) Target(obj, sym int) (Def, bool) {
	if obj < 0 || obj >= len(r.objs) || sym < 0 || sym >= len(r.objs[obj].Symbols) {
		return Def{}, false
	}
	s := r.objs[obj].Symbols[sym]
	if s.Sec >= 0 {
		return Def{Obj: obj, Sec: s.Sec, Off: s.Value}, true
	}
	return r.Lookup(s.Name)
}

// ---- TEXE: the executable format -------------------------------------------
//
//   header (12 bytes): "TEXE", version u8 (=1), nseg u8 (1..4), reserved u16 (=0), entry u32
//   then nseg segment entries (20 bytes each), then segment contents:
//   segment: flags u32 (1=X, 2=W, 4=R), addr u32, offset u32, filesz u32, memsz u32
//   Bytes memsz-filesz at the end of a segment are zero (like bss).

const exeMagic = "TEXE"

const (
	PermX uint32 = 1
	PermW uint32 = 2
	PermR uint32 = 4
)

const maxAddr = 1 << 20 // no segment may end above this address
const minAddr = 0x1000  // the first page is never mapped

var (
	ErrPerm    = errors.New("bad segment permissions")
	ErrOverlap = errors.New("overlapping segments")
	ErrEntry   = errors.New("bad entry point")
)

type Segment struct {
	Flags, Addr, Off, FileSize, MemSize uint32
	Data                                []byte // the FileSize bytes from the file (a copy)
}

type Exe struct {
	Entry uint32
	Segs  []Segment
}

func ParseExe(b []byte) (*Exe, error) {
	if len(b) < 12 {
		return nil, fmt.Errorf("header needs 12 bytes, have %d: %w", len(b), ErrTruncated)
	}
	if string(b[0:4]) != exeMagic {
		return nil, fmt.Errorf("magic %q: %w", b[0:4], ErrMagic)
	}
	if b[4] != 1 {
		return nil, fmt.Errorf("version %d: %w", b[4], ErrVersion)
	}
	n := int(b[5])
	if n < 1 || n > 4 || b[6]|b[7] != 0 {
		return nil, fmt.Errorf("segment count %d or reserved bytes: %w", n, ErrCorrupt)
	}
	tableEnd := 12 + 20*n
	if len(b) < tableEnd {
		return nil, fmt.Errorf("segment table needs %d bytes, have %d: %w", tableEnd, len(b), ErrTruncated)
	}
	exe := &Exe{Entry: le32(b[8:])}
	for i := 0; i < n; i++ {
		e := b[12+20*i:][:20]
		s := Segment{Flags: le32(e[0:]), Addr: le32(e[4:]), Off: le32(e[8:]), FileSize: le32(e[12:]), MemSize: le32(e[16:])}
		if s.Flags&^7 != 0 || s.Flags&PermR == 0 {
			return nil, fmt.Errorf("segment %d: flags %#x: %w", i, s.Flags, ErrCorrupt)
		}
		if s.Flags&PermW != 0 && s.Flags&PermX != 0 {
			return nil, fmt.Errorf("segment %d is writable and executable: %w", i, ErrPerm)
		}
		if s.MemSize == 0 || s.FileSize > s.MemSize || s.Addr < minAddr || uint64(s.Addr)+uint64(s.MemSize) > maxAddr {
			return nil, fmt.Errorf("segment %d: bad sizes or address: %w", i, ErrCorrupt)
		}
		if uint64(s.Off) < uint64(tableEnd) {
			return nil, fmt.Errorf("segment %d: contents inside the headers: %w", i, ErrCorrupt)
		}
		if uint64(s.Off)+uint64(s.FileSize) > uint64(len(b)) {
			return nil, fmt.Errorf("segment %d: contents past end of file: %w", i, ErrTruncated)
		}
		for j, p := range exe.Segs {
			if s.Addr < p.Addr+p.MemSize && p.Addr < s.Addr+s.MemSize {
				return nil, fmt.Errorf("segments %d and %d overlap: %w", j, i, ErrOverlap)
			}
		}
		s.Data = append([]byte{}, b[s.Off:s.Off+s.FileSize]...)
		exe.Segs = append(exe.Segs, s)
	}
	ok := false
	for _, s := range exe.Segs {
		if s.Flags&PermX != 0 && exe.Entry >= s.Addr && exe.Entry-s.Addr < s.MemSize {
			ok = true
		}
	}
	if !ok {
		return nil, fmt.Errorf("entry %#x is not inside an executable segment: %w", exe.Entry, ErrEntry)
	}
	return exe, nil
}

func perms(f uint32) string {
	out := []byte("---")
	if f&PermR != 0 {
		out[0] = 'R'
	}
	if f&PermW != 0 {
		out[1] = 'W'
	}
	if f&PermX != 0 {
		out[2] = 'X'
	}
	return string(out)
}

func Describe(e *Exe) string {
	var sb strings.Builder
	fmt.Fprintf(&sb, "TEXE entry=0x%08x segments=%d\n", e.Entry, len(e.Segs))
	for i, s := range e.Segs {
		fmt.Fprintf(&sb, "seg%d %s 0x%08x-0x%08x file=%d zero=%d\n", i, perms(s.Flags), s.Addr, s.Addr+s.MemSize, s.FileSize, s.MemSize-s.FileSize)
	}
	return sb.String()
}

var (
	ErrTooBig     = errors.New("image too big")
	ErrRelocRange = errors.New("relocation out of range")
)

const (
	Base     = 0x1000 // address of the first text byte
	PageSize = 0x100  // the data segment starts on a page boundary
)

type Plan struct {
	TextAddr, TextSize uint32
	DataAddr, DataSize uint32 // DataSize: initialised bytes
	MemSize            uint32 // size of the whole data segment, bss included
	addr               [][]uint32
}

func (p *Plan) Addr(obj, sec int) uint32 { return p.addr[obj][sec] }

func alignUp(v, a uint64) uint64 { return (v + a - 1) / a * a }

func Layout(objs []*Object) (*Plan, error) {
	p := &Plan{addr: make([][]uint32, len(objs))}
	for i, o := range objs {
		p.addr[i] = make([]uint32, len(o.Sections))
	}
	place := func(kind uint8, start uint64, cur uint64) (uint64, error) {
		for i, o := range objs {
			for j, s := range o.Sections {
				if s.Kind != kind {
					continue
				}
				cur = alignUp(cur, 4)
				if start+cur+uint64(s.Size) > maxAddr {
					return 0, fmt.Errorf("object %d section %d: %w", i, j, ErrTooBig)
				}
				p.addr[i][j] = uint32(start + cur)
				cur += uint64(s.Size)
			}
		}
		return cur, nil
	}
	n, err := place(SecText, Base, 0)
	if err != nil {
		return nil, err
	}
	p.TextAddr, p.TextSize = Base, uint32(n)
	dataAddr := alignUp(Base+n, PageSize)
	if dataAddr > maxAddr {
		return nil, ErrTooBig
	}
	p.DataAddr = uint32(dataAddr)
	d, err := place(SecData, dataAddr, 0)
	if err != nil {
		return nil, err
	}
	p.DataSize = uint32(d)
	m, err := place(SecBss, dataAddr, d)
	if err != nil {
		return nil, err
	}
	p.MemSize = uint32(m)
	return p, nil
}

type Image struct {
	Plan       *Plan
	Text, Data []byte // Text has Plan.TextSize bytes, Data has Plan.DataSize bytes
}

func Relocate(objs []*Object, res *Resolution, plan *Plan) (*Image, error) {
	img := &Image{Plan: plan, Text: make([]byte, plan.TextSize), Data: make([]byte, plan.DataSize)}
	where := func(i, sec int) []byte {
		if objs[i].Sections[sec].Kind == SecText {
			return img.Text[plan.Addr(i, sec)-plan.TextAddr:]
		}
		return img.Data[plan.Addr(i, sec)-plan.DataAddr:]
	}
	for i, o := range objs {
		for j, s := range o.Sections {
			if s.Kind != SecBss {
				copy(where(i, j), s.Data)
			}
		}
	}
	for i, o := range objs {
		for k, r := range o.Relocs {
			d, ok := res.Target(i, r.Sym)
			if !ok {
				return nil, fmt.Errorf("object %d reloc %d: %w", i, k, ErrUndefined)
			}
			s := int64(plan.Addr(d.Obj, d.Sec)) + int64(d.Off)
			p := int64(plan.Addr(i, r.Sec)) + int64(r.Off)
			field := where(i, r.Sec)[r.Off:]
			switch r.Kind {
			case RelAbs32:
				v := s + int64(r.Addend)
				if v < 0 || v > math.MaxUint32 {
					return nil, fmt.Errorf("object %d reloc %d: %d does not fit 32 bits: %w", i, k, v, ErrRelocRange)
				}
				binary.LittleEndian.PutUint32(field, uint32(v))
			case RelRel32:
				v := s + int64(r.Addend) - (p + 4)
				if v < math.MinInt32 || v > math.MaxInt32 {
					return nil, fmt.Errorf("object %d reloc %d: %d does not fit 32 signed bits: %w", i, k, v, ErrRelocRange)
				}
				binary.LittleEndian.PutUint32(field, uint32(int32(v)))
			}
		}
	}
	return img, nil
}

// ---- The TVM machine (provided) ---------------------------------------------
//
// Four 32-bit registers r0..r3, a return stack of at most 16 addresses and the
// memory described by the executable's segments. Opcodes (all integers little-endian):
//   00 HALT            01 r imm32  LI       02 r addr32 LOAD   03 r addr32 STORE
//   04 a b ADD (a+=b)  05 a b SUB          06 rel32 CALL       07 RET
//   08 r rel32 JNZ     09 r OUT            0A rel32 JMP
// rel32 is relative to the address just after the 4-byte field. Fetching needs an
// X segment, STORE needs W, LOAD needs R. Anything else is an ErrFault.

var (
	ErrFault = errors.New("machine fault")
	ErrSteps = errors.New("step limit reached")
)

func RunExe(b []byte, maxSteps int) ([]uint32, error) {
	exe, err := ParseExe(b)
	if err != nil {
		return nil, err
	}
	mem := make([]byte, maxAddr)
	for _, s := range exe.Segs {
		copy(mem[s.Addr:], s.Data)
	}
	pc := exe.Entry
	var reg [4]uint32
	var calls, out []uint32
	fault := func(f string, a ...any) error {
		return fmt.Errorf("at %#x: %s: %w", pc, fmt.Sprintf(f, a...), ErrFault)
	}
	can := func(addr, n, need uint32) bool {
		for _, s := range exe.Segs {
			if s.Flags&need != 0 && addr >= s.Addr && uint64(addr)+uint64(n) <= uint64(s.Addr)+uint64(s.MemSize) {
				return true
			}
		}
		return false
	}
	lens := map[byte]uint32{0: 1, 1: 6, 2: 6, 3: 6, 4: 3, 5: 3, 6: 5, 7: 1, 8: 6, 9: 2, 10: 5}
	for steps := 0; ; steps++ {
		if steps >= maxSteps {
			return out, ErrSteps
		}
		if !can(pc, 1, PermX) {
			return out, fault("fetch outside executable memory")
		}
		n, known := lens[mem[pc]]
		if !known {
			return out, fault("unknown opcode %#x", mem[pc])
		}
		if !can(pc, n, PermX) {
			return out, fault("instruction runs past executable memory")
		}
		in := mem[pc : pc+n]
		next := pc + n
		r := func(i int) (*uint32, bool) {
			if in[i] >= 4 {
				return nil, false
			}
			return &reg[in[i]], true
		}
		switch in[0] {
		case 0:
			return out, nil
		case 1, 2, 3:
			d, ok := r(1)
			if !ok {
				return out, fault("bad register")
			}
			v := le32(in[2:])
			switch in[0] {
			case 1:
				*d = v
			case 2:
				if !can(v, 4, PermR) {
					return out, fault("load from unmapped %#x", v)
				}
				*d = le32(mem[v:])
			case 3:
				if !can(v, 4, PermW) {
					return out, fault("store to unwritable %#x", v)
				}
				binary.LittleEndian.PutUint32(mem[v:], *d)
			}
		case 4, 5:
			a, ok1 := r(1)
			b2, ok2 := r(2)
			if !ok1 || !ok2 {
				return out, fault("bad register")
			}
			if in[0] == 4 {
				*a += *b2
			} else {
				*a -= *b2
			}
		case 6, 10:
			t := next + le32(in[1:])
			if in[0] == 6 {
				if len(calls) >= 16 {
					return out, fault("call stack overflow")
				}
				calls = append(calls, next)
			}
			next = t
		case 7:
			if len(calls) == 0 {
				return out, fault("return with empty call stack")
			}
			next = calls[len(calls)-1]
			calls = calls[:len(calls)-1]
		case 8:
			d, ok := r(1)
			if !ok {
				return out, fault("bad register")
			}
			if *d != 0 {
				next += le32(in[2:])
			}
		case 9:
			d, ok := r(1)
			if !ok {
				return out, fault("bad register")
			}
			if len(out) >= 1000 {
				return out, fault("too much output")
			}
			out = append(out, *d)
		}
		pc = next
	}
}

// WriteObject encodes an Object in the TOBJ format (provided: it is how the
// public example builds its input).
func WriteObject(o *Object) []byte {
	str := []byte{0}
	offs := map[string]uint32{"": 0}
	intern := func(s string) uint32 {
		if off, ok := offs[s]; ok {
			return off
		}
		offs[s] = uint32(len(str))
		str = append(append(str, s...), 0)
		return offs[s]
	}
	for _, s := range o.Sections {
		intern(s.Name)
	}
	for _, s := range o.Symbols {
		intern(s.Name)
	}
	u32 := func(b []byte, vs ...uint32) []byte {
		for _, v := range vs {
			b = binary.LittleEndian.AppendUint32(b, v)
		}
		return b
	}
	end := 16 + 16*len(o.Sections) + 12*len(o.Symbols) + 16*len(o.Relocs) + len(str)
	out := append([]byte(objMagic), 1, 0)
	out = binary.LittleEndian.AppendUint16(out, uint16(len(o.Sections)))
	out = binary.LittleEndian.AppendUint16(out, uint16(len(o.Symbols)))
	out = binary.LittleEndian.AppendUint16(out, uint16(len(o.Relocs)))
	out = u32(out, uint32(len(str)))
	var content []byte
	for _, s := range o.Sections {
		out = u32(out, intern(s.Name), uint32(s.Kind))
		off := uint32(0)
		if s.Kind != SecBss {
			off = uint32(end + len(content))
			content = append(content, s.Data...)
		}
		out = u32(out, off, s.Size)
	}
	for _, s := range o.Symbols {
		sec := uint32(noSection)
		if s.Sec >= 0 {
			sec = uint32(s.Sec)
		}
		out = u32(out, intern(s.Name), sec|uint32(s.Bind)<<16, s.Value)
	}
	for _, r := range o.Relocs {
		out = u32(out, uint32(r.Sec)|uint32(r.Kind)<<16, r.Off, uint32(r.Sym), uint32(r.Addend))
	}
	return append(append(out, str...), content...)
}

type MapEntry struct {
	Name string
	Addr uint32
}


// Emit writes the TEXE file for an image: a text segment (R+X), then, only if
// Plan.MemSize > 0, a data segment (R+W) with Plan.DataSize file bytes and
// Plan.MemSize bytes in memory. The header, then the segment table, then the text bytes, then
// the data bytes follow each other with no padding. entry must lie inside the text
// segment, otherwise ErrEntry.
func Emit(img *Image, entry uint32) ([]byte, error) {
	return nil, nil
}

// LinkMap lists every defined global symbol with its final address, sorted by
// address and then by name.
func LinkMap(objs []*Object, plan *Plan) []MapEntry {
	return nil
}

// Linked is the result of a successful link.
type Linked struct {
	Exe []byte     // the TEXE file
	Map []MapEntry // global symbols, see LinkMap
}

// Run is a linked program together with what happened when it ran.
type Run struct {
	Linked
	Description string   // Describe of the parsed executable
	Output      []uint32 // everything the program printed with OUT
}

// Link turns TOBJ files into an executable whose entry is the global
// function named entry. Parse each object (failures read "object N: ..." and still
// wrap the cause), then Resolve, Layout, Relocate, check the entry and Emit.
func Link(objs [][]byte, entry string) (*Linked, error) {
	return nil, nil
}

// LinkAndRun links, inspects the result with ParseExe and Describe, and runs it
// for at most maxSteps instructions. A link failure returns nil and the error. If the
// program faults or runs out of steps, return the Run (with the output so far) together
// with the error.
func LinkAndRun(objs [][]byte, entry string, maxSteps int) (*Run, error) {
	return nil, nil
}

// Try it with Run: main loads 21, calls double from another object, prints it.
func main() {
	main_ := &Object{
		Sections: []Section{{Name: ".text", Kind: SecText, Size: 10, Data: []byte{
			1, 0, 21, 0, 0, 0, // LI r0, 21
			6, 0, 0, 0, 0, // CALL double (patched)
			9, 0, // OUT r0
			0, // HALT
		}}},
		Symbols: []Symbol{{Name: "_start", Sec: 0, Bind: BindGlobal}, {Name: "double", Sec: -1, Bind: BindGlobal}},
		Relocs:  []Reloc{{Sec: 0, Kind: RelRel32, Off: 7, Sym: 1}},
	}
	main_.Sections[0].Size = uint32(len(main_.Sections[0].Data))
	lib := &Object{
		Sections: []Section{{Name: ".text", Kind: SecText, Size: 4, Data: []byte{4, 0, 0, 7}}},
		Symbols:  []Symbol{{Name: "double", Sec: 0, Bind: BindGlobal}},
	}
	run, err := LinkAndRun([][]byte{WriteObject(main_), WriteObject(lib)}, "_start", 1000)
	if err != nil {
		fmt.Println("error:", err)
		return
	}
	fmt.Print(run.Description)
	for _, m := range run.Map {
		fmt.Printf("%08x %s\n", m.Addr, m.Name)
	}
	fmt.Println("output:", run.Output)
}
