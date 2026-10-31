package main

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"sort"
	"strings"
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

var severity = map[string]string{"secret-read": "high", "periodic-beacon": "medium", "delayed-start": "low"}

func Detect(trace []Event, p Policy) ([]Finding, error) {
	out := []Finding{}
	// prefix[i] = total wait ticks in events before index i.
	prefix := make([]uint64, len(trace)+1)
	emits := map[string][]int{}
	var order []string
	first := -1
	for i, e := range trace {
		prefix[i+1] = prefix[i]
		switch e.Kind {
		case "wait":
			prefix[i+1] += uint64(e.Ticks)
			continue
		case "read":
			if strings.HasPrefix(e.Text, "secret/") {
				out = append(out, Finding{"secret-read", i, "high"})
			}
		case "emit":
			if _, ok := emits[e.Text]; !ok {
				order = append(order, e.Text)
			}
			emits[e.Text] = append(emits[e.Text], i)
		default:
			return nil, fmt.Errorf("unknown event kind")
		}
		if first < 0 {
			first = i
		}
	}
	if first >= 0 && prefix[first] > uint64(p.DelayTicks) {
		out = append(out, Finding{"delayed-start", first, "low"})
	}
	for _, text := range order {
		if p.Heartbeats[text] {
			continue
		}
		at := emits[text]
		gap := func(j int) uint64 { return prefix[at[j+1]] - prefix[at[j]] }
		for j := 0; j+2 < len(at); j++ {
			if g := gap(j); g > 0 && g == gap(j+1) {
				out = append(out, Finding{"periodic-beacon", at[j], "medium"})
				break
			}
		}
	}
	sort.Slice(out, func(a, b int) bool {
		if out[a].Index != out[b].Index {
			return out[a].Index < out[b].Index
		}
		return out[a].ID < out[b].ID
	})
	return out, nil
}

func ParseReport(data []byte) ([]Finding, error) {
	var doc struct {
		Schema   *int `json:"schema"`
		Findings *[]struct {
			ID       string  `json:"id"`
			Index    int     `json:"index"`
			Severity *string `json:"severity"`
		} `json:"findings"`
	}
	dec := json.NewDecoder(bytes.NewReader(data))
	dec.DisallowUnknownFields()
	if err := dec.Decode(&doc); err != nil {
		return nil, err
	}
	if _, err := dec.Token(); err != io.EOF {
		return nil, fmt.Errorf("trailing data")
	}
	if doc.Schema == nil || doc.Findings == nil || (*doc.Schema != 1 && *doc.Schema != 2) || len(*doc.Findings) > 64 {
		return nil, fmt.Errorf("invalid report")
	}
	out := []Finding{}
	for _, f := range *doc.Findings {
		sev, ok := severity[f.ID]
		if !ok || f.Index < 0 {
			return nil, fmt.Errorf("invalid finding")
		}
		if *doc.Schema == 1 && f.Severity != nil {
			return nil, fmt.Errorf("severity not in schema 1")
		}
		if *doc.Schema == 2 && (f.Severity == nil || *f.Severity != sev) {
			return nil, fmt.Errorf("bad severity")
		}
		out = append(out, Finding{f.ID, f.Index, sev})
	}
	return out, nil
}

func main() {
	prog := []Instruction{{Op: OpRead, Text: "secret/lab"}, {Op: OpEmit, Text: "hi"}, {Op: OpHalt}}
	fmt.Println(Detect(Execute(prog, map[string]string{"secret/lab": "x"}), Policy{}))
	fmt.Println(ParseReport([]byte(`{"schema":1,"findings":[{"id":"delayed-start","index":2}]}`)))
}
