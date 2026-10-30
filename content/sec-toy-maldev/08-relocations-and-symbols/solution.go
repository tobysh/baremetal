package main

import (
	"encoding/binary"
	"fmt"
	"hash/crc32"
)

type Reloc struct {
	Site   int
	Symbol string
}

type Object struct {
	Code    []byte
	Exports map[string]int
	Relocs  []Reloc
}

type decoded struct {
	starts map[int]bool
	calls  map[int]bool // operand offsets of CALLs
	count  int
	halts  int
}

func decode(code []byte) (decoded, bool) {
	d := decoded{starts: map[int]bool{}, calls: map[int]bool{}}
	last := byte(255)
	for pc := 0; pc < len(code); {
		d.starts[pc] = true
		op := code[pc]
		last = op
		pc++
		switch op {
		case 0:
			d.halts++
		case 1, 2:
			if pc >= len(code) {
				return d, false
			}
			n := int(code[pc])
			pc++
			if n > len(code)-pc || (op == 2 && n == 0) {
				return d, false
			}
			pc += n
		case 3:
			if len(code)-pc < 2 || (code[pc] == 0 && code[pc+1] == 0) {
				return d, false
			}
			pc += 2
		case 4:
			if len(code)-pc < 2 || code[pc] != 0 || code[pc+1] != 0 {
				return d, false
			}
			d.calls[pc] = true
			pc += 2
		default:
			return d, false
		}
		d.count++
	}
	return d, last == 0 && len(code) > 0
}

func Link(objs []Object) ([]byte, error) {
	bad := func(s string) ([]byte, error) { return nil, fmt.Errorf("link: %s", s) }
	if len(objs) == 0 || len(objs) > 8 {
		return bad("object count")
	}
	infos := make([]decoded, len(objs))
	bases := make([]int, len(objs))
	total, count, halts, calls := 0, 0, 0, 0
	for i, o := range objs {
		d, ok := decode(o.Code)
		if !ok {
			return bad("malformed code")
		}
		infos[i] = d
		bases[i] = total
		total += len(o.Code)
		count += d.count
		halts += d.halts
		calls += len(d.calls)
		if total > 4096 || count > 64 {
			return bad("too large")
		}
	}
	symbols := map[string]int{}
	for i, o := range objs {
		for name, off := range o.Exports {
			if len(name) == 0 || len(name) > 32 || !infos[i].starts[off] {
				return bad("bad export")
			}
			if _, dup := symbols[name]; dup {
				return bad("duplicate symbol")
			}
			symbols[name] = bases[i] + off
		}
	}
	entry, ok := symbols["main"]
	if !ok {
		return bad("no main")
	}
	out := make([]byte, 0, total)
	for _, o := range objs {
		out = append(out, o.Code...)
	}
	for i, o := range objs {
		done := map[int]bool{}
		for _, r := range o.Relocs {
			if !infos[i].calls[r.Site] || done[r.Site] {
				return bad("bad relocation site")
			}
			done[r.Site] = true
			target, ok := symbols[r.Symbol]
			if !ok {
				return bad("unresolved symbol")
			}
			binary.LittleEndian.PutUint16(out[bases[i]+r.Site:], uint16(target))
		}
		if len(done) != len(infos[i].calls) {
			return bad("unresolved call")
		}
	}
	magic := "BML2"
	if calls == 0 && halts == 1 {
		magic = "BML1"
	}
	capsule := make([]byte, 12, 12+len(out))
	copy(capsule, magic)
	binary.LittleEndian.PutUint16(capsule[4:], uint16(entry))
	binary.LittleEndian.PutUint16(capsule[6:], uint16(len(out)))
	binary.LittleEndian.PutUint32(capsule[8:], crc32.ChecksumIEEE(out))
	return append(capsule, out...), nil
}

func main() {
	a := Object{Code: []byte{4, 0, 0, 0}, Exports: map[string]int{"main": 0}, Relocs: []Reloc{{Site: 1, Symbol: "f"}}}
	b := Object{Code: []byte{1, 2, 'h', 'i', 0}, Exports: map[string]int{"f": 0}}
	out, err := Link([]Object{a, b})
	fmt.Printf("% x (%v)\n", out, err)
}
