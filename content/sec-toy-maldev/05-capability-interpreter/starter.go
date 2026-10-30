package main

import (
	"fmt"
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
func Run(program []Instruction, caps map[string]string, maxOps int, maxTicks uint32) (Trace, error) {
	return Trace{}, nil
}

func main() {
	fmt.Println(Run([]Instruction{{Op: OpRead, Text: "note"}, {Op: OpHalt}}, map[string]string{"note": "lab data"}, 2, 0))
}
