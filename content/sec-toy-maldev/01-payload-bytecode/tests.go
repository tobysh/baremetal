package main

import (
	"bytes"
	"fmt"
	"reflect"
	"testing"
)

func TestAssemblyExamples(t *testing.T) {
	p := []Instruction{{Op: OpEmit, Text: "hi"}, {Op: OpWait, Ticks: 258}, {Op: OpHalt}}
	got, err := Assemble(p)
	if err != nil || !bytes.Equal(got, []byte{1, 2, 'h', 'i', 3, 2, 1, 0}) {
		t.Fatal(got, err)
	}
}
func TestAssemblyVariedOperands(t *testing.T) {
	for i := 0; i < 40; i++ {
		text := fmt.Sprintf("m%d\x00é", i)
		key := fmt.Sprint("lab-key-", i)
		ticks := uint16(1 + i*997)
		p := []Instruction{{Op: OpEmit, Text: text}, {Op: OpRead, Text: key}, {Op: OpWait, Ticks: ticks}, {Op: OpHalt}}
		before := append([]Instruction{}, p...)
		got, err := Assemble(p)
		want := []byte{1, byte(len(text))}
		want = append(want, []byte(text)...)
		want = append(want, 2, byte(len(key)))
		want = append(want, []byte(key)...)
		want = append(want, 3, byte(ticks), byte(ticks>>8), 0)
		if err != nil || !bytes.Equal(got, want) || !reflect.DeepEqual(p, before) {
			t.Fatal(i, got, err)
		}
		if len(got) > 0 {
			got[0] = 99
		}
		again, _ := Assemble(p)
		if !bytes.Equal(again, want) {
			t.Fatal("shared output storage")
		}
	}
}
func TestAssemblyRejectsInvalid(t *testing.T) {
	bad := [][]Instruction{nil, {{Op: OpEmit}}, {{Op: OpHalt}, {Op: OpHalt}}, {{Op: 99}, {Op: OpHalt}}, {{Op: OpRead}, {Op: OpHalt}}, {{Op: OpWait}, {Op: OpHalt}}, {{Op: OpWait, Text: "x", Ticks: 1}, {Op: OpHalt}}, {{Op: OpEmit, Ticks: 1}, {Op: OpHalt}}, {{Op: OpHalt, Text: "x"}}, {{Op: OpHalt, Ticks: 1}}, {{Op: OpEmit, Text: string(bytes.Repeat([]byte{'x'}, 256))}, {Op: OpHalt}}}
	p := make([]Instruction, 65)
	for i := range p {
		p[i] = Instruction{Op: OpEmit}
	}
	p[64] = Instruction{Op: OpHalt}
	bad = append(bad, p)
	for i, p := range bad {
		out, err := Assemble(p)
		if err == nil || out != nil {
			t.Fatal("accepted invalid program", i)
		}
	}
}
func TestAssemblyBoundaries(t *testing.T) {
	p := make([]Instruction, 64)
	for i := 0; i < 63; i++ {
		p[i] = Instruction{Op: OpEmit, Text: string(bytes.Repeat([]byte{byte(i)}, 255))}
	}
	p[63] = Instruction{Op: OpHalt}
	out, err := Assemble(p)
	if err != nil || len(out) != 63*257+1 {
		t.Fatal(len(out), err)
	}
	if out, err := Assemble([]Instruction{{Op: OpEmit}, {Op: OpHalt}}); err != nil || !bytes.Equal(out, []byte{1, 0, 0}) {
		t.Fatal(out, err)
	}
}
