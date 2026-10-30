package main

import (
	"encoding/binary"
	"fmt"
	"hash/crc32"
)

type Instruction struct {
	Op    byte
	Text  string
	Ticks uint16
}

const (
	OpHalt byte = iota
	OpEmit
	OpRead
	OpWait
)

type Image struct {
	Instructions []Instruction
	Entry        int
}

func LoadCapsule(blob []byte) (Image, error) {
	fail := func() (Image, error) { return Image{}, fmt.Errorf("invalid capsule") }
	if len(blob) < 12 || string(blob[:4]) != "BML1" {
		return fail()
	}
	n := int(binary.LittleEndian.Uint16(blob[6:8]))
	entry := int(binary.LittleEndian.Uint16(blob[4:6]))
	if n == 0 || n > 4096 || len(blob) != 12+n || entry >= n {
		return fail()
	}
	code := blob[12:]
	if crc32.ChecksumIEEE(code) != binary.LittleEndian.Uint32(blob[8:12]) {
		return fail()
	}
	program := []Instruction{}
	offsets := map[int]int{}
	for pc := 0; pc < len(code); {
		offsets[pc] = len(program)
		op := Instruction{Op: code[pc]}
		pc++
		switch op.Op {
		case OpHalt:
			if pc != len(code) {
				return fail()
			}
		case OpEmit, OpRead:
			if pc >= len(code) {
				return fail()
			}
			size := int(code[pc])
			pc++
			if size > len(code)-pc || (op.Op == OpRead && size == 0) {
				return fail()
			}
			op.Text = string(code[pc : pc+size])
			pc += size
		case OpWait:
			if len(code)-pc < 2 {
				return fail()
			}
			op.Ticks = binary.LittleEndian.Uint16(code[pc : pc+2])
			pc += 2
			if op.Ticks == 0 {
				return fail()
			}
		default:
			return fail()
		}
		program = append(program, op)
		if len(program) > 64 {
			return fail()
		}
	}
	if len(program) == 0 || program[len(program)-1].Op != OpHalt {
		return fail()
	}
	idx, ok := offsets[entry]
	if !ok {
		return fail()
	}
	return Image{Instructions: program, Entry: idx}, nil
}

func main() {
	fmt.Println(LoadCapsule([]byte{66, 77, 76, 49, 4, 0, 8, 0, 20, 28, 184, 70, 1, 2, 104, 105, 3, 2, 1, 0}))
}
