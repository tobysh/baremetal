package main

import (
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

// Try it with Run.
func main() {
	obj := []byte("TOBJ\x01\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00")
	h, err := ParseHeader(obj)
	fmt.Printf("%+v %v\n", h, err)
	_, err = ParseHeader(obj[:10])
	fmt.Println(err, errors.Is(err, ErrTruncated))
	_, err = ParseHeader([]byte("ELF\x7f is not ours, not at all"))
	fmt.Println(err, errors.Is(err, ErrMagic))
}
