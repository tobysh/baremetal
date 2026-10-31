package main

import (
	"encoding/binary"
	"errors"
	"fmt"
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

// Try it with Run.
func main() {
	b := []byte("TEXE\x01\x01\x00\x00\x00\x10\x00\x00") // entry 0x1000, one segment
	b = append(b, 5, 0, 0, 0, 0, 0x10, 0, 0, 32, 0, 0, 0, 3, 0, 0, 0, 3, 0, 0, 0)
	b = append(b, 0, 0, 0) // three HALT instructions
	e, err := ParseExe(b)
	if err != nil {
		fmt.Println("error:", err)
		return
	}
	fmt.Print(Describe(e))
	b[8] = 0x20
	_, err = ParseExe(b)
	fmt.Println(err)
}
