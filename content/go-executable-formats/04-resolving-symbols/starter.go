package main

import (
	"bytes"
	"encoding/binary"
	"errors"
	"fmt"
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

// Resolve builds the global symbol table. Defined global symbols (Sec >= 0, BindGlobal)
// are entered by name; a second definition is ErrDuplicate (message names the symbol).
// After all definitions are in, every undefined symbol must be defined by some object,
// otherwise ErrUndefined. Report the first problem in object order, then symbol order,
// and report all duplicates before any undefined symbol. Locals are never exported.
func Resolve(objs []*Object) (*Resolution, error) {
	return nil, nil
}

// Lookup finds a global by name.
func (r *Resolution) Lookup(name string) (Def, bool) {
	return Def{}, false
}

// Target says where symbol sym of object obj lives: its own definition when it has one,
// otherwise the global with its name. ok is false for indexes out of range.
func (r *Resolution) Target(obj, sym int) (Def, bool) {
	return Def{}, false
}

// Try it with Run: object 0 calls "double", which object 1 defines.
func main() {
	caller := &Object{
		Sections: []Section{{Name: ".text", Kind: SecText, Data: make([]byte, 5), Size: 5}},
		Symbols:  []Symbol{{Name: "double", Sec: -1, Bind: BindGlobal}, {Name: "main", Sec: 0, Bind: BindGlobal}},
	}
	lib := &Object{
		Sections: []Section{{Name: ".text", Kind: SecText, Data: []byte{4, 0, 0, 7}, Size: 4}},
		Symbols:  []Symbol{{Name: "double", Sec: 0, Bind: BindGlobal}, {Name: "main", Sec: 0, Bind: BindLocal, Value: 1}},
	}
	res, err := Resolve([]*Object{caller, lib})
	fmt.Println(err)
	fmt.Println(res.Lookup("double"))
	fmt.Println(res.Target(0, 0)) // the caller's reference to double
	fmt.Println(res.Target(1, 1)) // lib's local main
	_, err = Resolve([]*Object{caller})
	fmt.Println(err)
	_, err = Resolve([]*Object{caller, lib, lib})
	fmt.Println(err)
}
