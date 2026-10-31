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

type Event struct {
	Kind  string
	Text  string
	Ticks uint32
}

type Finding struct {
	ID       string
	Index    int
	Severity string
}

type Policy struct {
	DelayTicks uint32
	Heartbeats map[string]bool
}

// Execute records the events of a program; it is supplied and not graded.
func Execute(program []Instruction, caps map[string]string) []Event {
	events := []Event{}
	for _, op := range program {
		switch op.Op {
		case OpEmit:
			events = append(events, Event{Kind: "emit", Text: op.Text})
		case OpRead:
			if _, ok := caps[op.Text]; ok {
				events = append(events, Event{Kind: "read", Text: op.Text})
			}
		case OpWait:
			events = append(events, Event{Kind: "wait", Ticks: uint32(op.Ticks)})
		case OpHalt:
			return events
		}
	}
	return events
}

func Detect(trace []Event, p Policy) ([]Finding, error) { return nil, nil }

func ParseReport(data []byte) ([]Finding, error) { return nil, nil }

func main() {
	prog := []Instruction{{Op: OpRead, Text: "secret/lab"}, {Op: OpEmit, Text: "hi"}, {Op: OpHalt}}
	fmt.Println(Detect(Execute(prog, map[string]string{"secret/lab": "x"}), Policy{}))
	fmt.Println(ParseReport([]byte(`{"schema":1,"findings":[{"id":"delayed-start","index":2}]}`)))
}
