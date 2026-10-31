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

// ReadU16 reads a little-endian uint16 at byte offset off. It must never
// panic: an offset at or beyond the end, or one so large that off+2 would
// wrap around, is an ErrTruncated error.
func ReadU16(b []byte, off uint64) (uint16, error) {
	return 0, nil
}

// ReadU32 is the same for a uint32.
func ReadU32(b []byte, off uint64) (uint32, error) {
	return 0, nil
}

// ParseHeader decodes and validates the 16-byte header and computes where
// each table starts. Check, in this order:
//   fewer than 16 bytes        -> ErrTruncated
//   magic is not "TOBJ"        -> ErrMagic
//   version is not 1           -> ErrVersion
//   flags is not 0             -> ErrCorrupt
//   tables reach past the end  -> ErrTruncated
func ParseHeader(b []byte) (Header, error) {
	return Header{}, nil
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
