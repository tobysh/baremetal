package main

import (
	"fmt"
	"reflect"
	"strings"
	"testing"
)

func TestInterpreterCapabilities(t *testing.T) {
	for i := 0; i < 24; i++ {
		key := fmt.Sprint("lab-key-", i)
		value := fmt.Sprintf("record-%d\x00", i)
		p := []Instruction{{Op: OpRead, Text: key}, {Op: OpWait, Ticks: uint16(i + 1)}, {Op: OpEmit, Text: "done"}, {Op: OpHalt}}
		caps := map[string]string{key: value}
		before := append([]Instruction{}, p...)
		got, err := Run(p, caps, 4, uint32(i+1))
		want := Trace{Outputs: []string{value, "done"}, Reads: []string{key}, Ticks: uint32(i + 1)}
		if err != nil || !reflect.DeepEqual(got, want) || !reflect.DeepEqual(p, before) || len(caps) != 1 || caps[key] != value {
			t.Fatal(got, err)
		}
		got, err = Run(p, map[string]string{}, 4, 100)
		if err == nil || !reflect.DeepEqual(got, Trace{}) {
			t.Fatal("unauthorized read accepted")
		}
	}
}
func TestInterpreterBudgetsAndNoPartialTrace(t *testing.T) {
	p := []Instruction{{Op: OpEmit, Text: "before"}, {Op: OpWait, Ticks: 9}, {Op: OpHalt}}
	for _, c := range []struct {
		ops   int
		ticks uint32
	}{{0, 100}, {2, 100}, {3, 8}} {
		got, err := Run(p, nil, c.ops, c.ticks)
		if err == nil || !reflect.DeepEqual(got, Trace{}) {
			t.Fatal("budget accepted", c, got)
		}
	}
	for _, p := range [][]Instruction{{{Op: OpEmit, Text: "before"}, {Op: OpRead, Text: "/etc/shadow"}, {Op: OpHalt}}, {{Op: OpHalt}, {Op: OpRead, Text: "x"}}, {{Op: 99}, {Op: OpHalt}}} {
		if got, err := Run(p, nil, 64, 100); err == nil || !reflect.DeepEqual(got, Trace{}) {
			t.Fatal("invalid or unauthorized program accepted")
		}
	}
}
func TestInterpreterOutputLimit(t *testing.T) {
	p := []Instruction{{Op: OpRead, Text: "blob"}, {Op: OpHalt}}
	for _, n := range []int{0, 4096, 4097} {
		caps := map[string]string{"blob": strings.Repeat("x", n)}
		got, err := Run(p, caps, 2, 0)
		if n <= 4096 {
			if err != nil || len(got.Outputs) != 1 || len(got.Outputs[0]) != n {
				t.Fatal(n, got, err)
			}
		} else if err == nil || !reflect.DeepEqual(got, Trace{}) {
			t.Fatal("unbounded output")
		}
	}
	p = []Instruction{{Op: OpRead, Text: "blob"}, {Op: OpRead, Text: "blob"}, {Op: OpHalt}}
	if got, err := Run(p, map[string]string{"blob": strings.Repeat("x", 2049)}, 3, 0); err == nil || !reflect.DeepEqual(got, Trace{}) {
		t.Fatal("cumulative output not bounded")
	}
}
func TestInterpreterHalt(t *testing.T) {
	got, err := Run([]Instruction{{Op: OpHalt}}, nil, 1, 0)
	if err != nil || got.Outputs == nil || got.Reads == nil || got.Ticks != 0 || len(got.Outputs) != 0 || len(got.Reads) != 0 {
		t.Fatal(got, err)
	}
}
