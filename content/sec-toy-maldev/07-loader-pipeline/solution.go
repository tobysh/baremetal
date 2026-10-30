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
type Trace struct {
	Outputs []string
	Reads   []string
	Ticks   uint32
}

func Assemble(program []Instruction) ([]byte, error) {
	if len(program) == 0 || len(program) > 64 {
		return nil, fmt.Errorf("invalid instruction count")
	}
	out := []byte{}
	for i, op := range program {
		switch op.Op {
		case OpHalt:
			if i != len(program)-1 || op.Text != "" || op.Ticks != 0 {
				return nil, fmt.Errorf("invalid HALT")
			}
			out = append(out, OpHalt)
		case OpEmit, OpRead:
			if i == len(program)-1 || op.Ticks != 0 || len(op.Text) > 255 || (op.Op == OpRead && len(op.Text) == 0) {
				return nil, fmt.Errorf("invalid text instruction")
			}
			out = append(out, op.Op, byte(len(op.Text)))
			out = append(out, []byte(op.Text)...)
		case OpWait:
			if i == len(program)-1 || op.Text != "" || op.Ticks == 0 {
				return nil, fmt.Errorf("invalid WAIT")
			}
			out = append(out, OpWait, byte(op.Ticks), byte(op.Ticks>>8))
		default:
			return nil, fmt.Errorf("unknown opcode")
		}
	}
	return out, nil
}

func Wrap(code []byte, entry uint16) ([]byte, error) {
	if len(code) == 0 || len(code) > 4096 || int(entry) >= len(code) {
		return nil, fmt.Errorf("invalid code range")
	}
	out := make([]byte, 12+len(code))
	copy(out, "BML1")
	binary.LittleEndian.PutUint16(out[4:6], entry)
	binary.LittleEndian.PutUint16(out[6:8], uint16(len(code)))
	binary.LittleEndian.PutUint32(out[8:12], crc32.ChecksumIEEE(code))
	copy(out[12:], code)
	return out, nil
}

func Pack(plain []byte, key byte) ([]byte, error) {
	if key == 0 || len(plain) == 0 || len(plain) > 8192 {
		return nil, fmt.Errorf("invalid package input")
	}
	out := make([]byte, 11+len(plain))
	copy(out, "BX01")
	binary.LittleEndian.PutUint16(out[4:6], uint16(len(plain)))
	out[6] = key
	binary.LittleEndian.PutUint32(out[7:11], crc32.ChecksumIEEE(plain))
	for i, b := range plain {
		out[11+i] = b ^ key
	}
	return out, nil
}
func Unpack(bundle []byte) ([]byte, error) {
	if len(bundle) < 11 || string(bundle[:4]) != "BX01" || bundle[6] == 0 {
		return nil, fmt.Errorf("invalid package header")
	}
	n := int(binary.LittleEndian.Uint16(bundle[4:6]))
	if n == 0 || n > 8192 || len(bundle) != 11+n {
		return nil, fmt.Errorf("invalid package length")
	}
	out := make([]byte, n)
	for i := range out {
		out[i] = bundle[11+i] ^ bundle[6]
	}
	if crc32.ChecksumIEEE(out) != binary.LittleEndian.Uint32(bundle[7:11]) {
		return nil, fmt.Errorf("package checksum mismatch")
	}
	return out, nil
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

func Run(program []Instruction, caps map[string]string, maxOps int, maxTicks uint32) (Trace, error) {
	fail := func() (Trace, error) { return Trace{}, fmt.Errorf("execution rejected") }
	if _, err := Assemble(program); err != nil {
		return fail()
	}
	if maxOps <= 0 || len(program) > maxOps {
		return fail()
	}
	out := Trace{Outputs: []string{}, Reads: []string{}}
	total := 0
	for _, op := range program {
		switch op.Op {
		case OpHalt:
			return out, nil
		case OpEmit:
			if len(op.Text) > 4096-total {
				return fail()
			}
			out.Outputs = append(out.Outputs, op.Text)
			total += len(op.Text)
		case OpRead:
			value, ok := caps[op.Text]
			if !ok || len(value) > 4096-total {
				return fail()
			}
			out.Reads = append(out.Reads, op.Text)
			out.Outputs = append(out.Outputs, value)
			total += len(value)
		case OpWait:
			if uint64(out.Ticks)+uint64(op.Ticks) > uint64(maxTicks) {
				return fail()
			}
			out.Ticks += uint32(op.Ticks)
		}
	}
	return fail()
}

func ExecuteBundle(bundle []byte, caps map[string]string, maxOps int, maxTicks uint32) (Trace, error) {
	plain, err := Unpack(bundle)
	if err != nil {
		return Trace{}, err
	}
	image, err := LoadCapsule(plain)
	if err != nil {
		return Trace{}, err
	}
	return Run(image.Instructions[image.Entry:], caps, maxOps, maxTicks)
}

func main() {
	code, _ := Assemble([]Instruction{{Op: OpRead, Text: "note"}, {Op: OpEmit, Text: "done"}, {Op: OpHalt}})
	capsule, _ := Wrap(code, 0)
	bundle, _ := Pack(capsule, 19)
	fmt.Println(ExecuteBundle(bundle, map[string]string{"note": "public lab data"}, 3, 0))
}
