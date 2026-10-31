package main

import (
	"fmt"
	"reflect"
	"sort"
	"strings"
	"testing"
)

func rd(k string) Event { return Event{Kind: "read", Text: k} }
func em(s string) Event { return Event{Kind: "emit", Text: s} }
func wt(n uint32) Event { return Event{Kind: "wait", Ticks: n} }
func F(id string, i int) Finding {
	return Finding{ID: id, Index: i, Severity: map[string]string{"secret-read": "high", "periodic-beacon": "medium", "delayed-start": "low"}[id]}
}

func detect(t *testing.T, tr []Event, p Policy) []Finding {
	t.Helper()
	before := append([]Event{}, tr...)
	got, err := Detect(tr, p)
	if err != nil || got == nil || !reflect.DeepEqual(before, append([]Event{}, tr...)) {
		t.Fatal("bad result", got, err)
	}
	return got
}

func TestSecretReadsAndMisleadingStrings(t *testing.T) {
	tr := []Event{rd("secret/lab"), em("see secret/lab docs"), rd("public/secret/x"), rd("Secret/a"), rd("secrets/a"), rd("secret"), rd("secret/"), em("secret/lab")}
	got := detect(t, tr, Policy{DelayTicks: 10})
	want := []Finding{F("secret-read", 0), F("secret-read", 6)}
	if !reflect.DeepEqual(got, want) {
		t.Fatal(got)
	}
	// Benign corpus: no secret findings for many other keys and texts.
	for i := 0; i < 30; i++ {
		tr := []Event{rd(fmt.Sprint("config/", i)), em(fmt.Sprint("secret/", i, " is documented")), rd(fmt.Sprint("secret", i))}
		if g := detect(t, tr, Policy{DelayTicks: 5}); len(g) != 0 {
			t.Fatal("false positive", g)
		}
	}
}

func TestBeaconRule(t *testing.T) {
	ping := func(gaps ...uint32) []Event {
		tr := []Event{em("ping")}
		for _, g := range gaps {
			tr = append(tr, wt(g), em("ping"))
		}
		return tr
	}
	for name, c := range map[string]struct {
		tr   []Event
		want []Finding
	}{
		"regular":                         {ping(30, 30), []Finding{F("periodic-beacon", 0)}},
		"regular split wait":              {append(append([]Event{em("ping"), wt(10), wt(20), em("ping")}, wt(30)), em("ping")), []Finding{F("periodic-beacon", 0)}},
		"starts later":                    {ping(5, 7, 7), []Finding{F("periodic-beacon", 2)}},
		"jitter":                          {ping(30, 31, 29, 30), nil},
		"zero gaps":                       {[]Event{em("ping"), em("ping"), em("ping"), em("ping")}, nil},
		"two only":                        {ping(30), nil},
		"other text between":              {[]Event{em("ping"), wt(4), em("x"), wt(1), em("ping"), wt(5), em("ping")}, []Finding{F("periodic-beacon", 0)}},
		"interleaved texts, a is regular": {[]Event{em("a"), wt(3), em("b"), wt(3), em("a"), wt(3), em("b"), wt(3), em("a")}, []Finding{F("periodic-beacon", 0)}},
		"gap uses all waits":              {[]Event{em("a"), wt(3), em("a"), wt(2), wt(1), em("a")}, []Finding{F("periodic-beacon", 0)}},
		"not consecutive":                 {[]Event{em("a"), wt(3), em("a"), wt(4), em("a"), wt(3), em("a")}, nil},
	} {
		got := detect(t, c.tr, Policy{DelayTicks: 1000})
		if c.want == nil {
			c.want = []Finding{}
		}
		if !reflect.DeepEqual(got, c.want) {
			t.Fatal(name, got, c.want)
		}
	}
	// Expected heartbeat: suppressed for that exact text only.
	tr := append(ping(30, 30), rd("secret/k"), wt(2), em("ping2"), wt(2), em("ping2"), wt(2), em("ping2"))
	got := detect(t, tr, Policy{DelayTicks: 1000, Heartbeats: map[string]bool{"ping": true}})
	want := []Finding{F("secret-read", 5), F("periodic-beacon", 7)}
	if !reflect.DeepEqual(got, want) {
		t.Fatal(got)
	}
	if g := detect(t, ping(30, 30), Policy{DelayTicks: 1000, Heartbeats: map[string]bool{"pin": true, "ping ": true}}); len(g) != 1 {
		t.Fatal("heartbeat match must be exact", g)
	}
	// Large sums do not wrap.
	big := []Event{em("x"), wt(1 << 31), wt(1 << 31), em("x"), wt(0), wt(1 << 31), wt(1 << 31), em("x")}
	if g := detect(t, big, Policy{DelayTicks: 1}); !reflect.DeepEqual(g, []Finding{F("periodic-beacon", 0)}) {
		t.Fatal(g)
	}
}

func TestDelayedStart(t *testing.T) {
	for _, c := range []struct {
		tr    []Event
		delay uint32
		want  []Finding
	}{
		{[]Event{wt(501), em("x")}, 500, []Finding{F("delayed-start", 1)}},
		{[]Event{wt(500), em("x")}, 500, nil},
		{[]Event{wt(300), wt(300), rd("config/a")}, 500, []Finding{F("delayed-start", 2)}},
		{[]Event{em("x"), wt(9999), em("y")}, 5, nil},
		{[]Event{wt(9999)}, 5, nil},
		{nil, 0, nil},
		{[]Event{wt(1), em("x")}, 0, []Finding{F("delayed-start", 1)}},
		{[]Event{wt(1 << 31), wt(1 << 31), em("x")}, 1 << 31, []Finding{F("delayed-start", 2)}},
	} {
		got := detect(t, c.tr, Policy{DelayTicks: c.delay})
		if c.want == nil {
			c.want = []Finding{}
		}
		if !reflect.DeepEqual(got, c.want) {
			t.Fatal(c.tr, got, c.want)
		}
	}
	// Sorted output across rules.
	tr := []Event{wt(100), rd("secret/a"), em("p"), wt(2), em("p"), wt(2), em("p")}
	want := []Finding{F("delayed-start", 1), F("secret-read", 1), F("periodic-beacon", 2)}
	if got := detect(t, tr, Policy{DelayTicks: 10}); !reflect.DeepEqual(got, want) {
		t.Fatal(got)
	}
}

func TestUnknownKind(t *testing.T) {
	for _, tr := range [][]Event{{{Kind: "exec", Text: "x"}}, {em("a"), {Kind: ""}}, {{Kind: "READ", Text: "secret/a"}}} {
		if g, err := Detect(tr, Policy{}); err == nil || g != nil {
			t.Fatal("unknown kind accepted", g)
		}
	}
}

// reference is a clock-based re-statement of the rules used on random traces.
func reference(tr []Event, p Policy) []Finding {
	out := []Finding{}
	var clock uint64
	type stamp struct {
		idx int
		at  uint64
	}
	times := map[string][]stamp{}
	started := false
	for i, e := range tr {
		if e.Kind == "wait" {
			clock += uint64(e.Ticks)
			continue
		}
		if !started {
			started = true
			if clock > uint64(p.DelayTicks) {
				out = append(out, F("delayed-start", i))
			}
		}
		if e.Kind == "read" && strings.HasPrefix(e.Text, "secret/") {
			out = append(out, F("secret-read", i))
		}
		if e.Kind == "emit" {
			times[e.Text] = append(times[e.Text], stamp{i, clock})
		}
	}
	for text, s := range times {
		if p.Heartbeats[text] {
			continue
		}
		for j := 0; j+2 < len(s); j++ {
			if g := s[j+1].at - s[j].at; g > 0 && s[j+2].at-s[j+1].at == g {
				out = append(out, F("periodic-beacon", s[j].idx))
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
	return out
}

func TestAgainstReferenceOnRandomTraces(t *testing.T) {
	seed := uint32(7)
	next := func(n int) int { seed = seed*1664525 + 1013904223; return int(seed>>10) % n }
	texts := []string{"a", "b", "ping", "ok"}
	keys := []string{"secret/k", "public/k", "config/x", "secrets/y"}
	for i := 0; i < 400; i++ {
		var tr []Event
		for n := next(14); n >= 0; n-- {
			switch next(3) {
			case 0:
				tr = append(tr, em(texts[next(len(texts))]))
			case 1:
				tr = append(tr, rd(keys[next(len(keys))]))
			default:
				tr = append(tr, wt(uint32(next(4))*uint32(1+next(3))))
			}
		}
		p := Policy{DelayTicks: uint32(next(6))}
		if next(4) == 0 {
			p.Heartbeats = map[string]bool{"ping": true}
		}
		got, want := detect(t, tr, p), reference(tr, p)
		if !reflect.DeepEqual(got, want) {
			t.Fatalf("trace %v policy %v\n got %v\nwant %v", tr, p, got, want)
		}
	}
}

func TestPipelineFromPrograms(t *testing.T) {
	caps := map[string]string{"secret/lab": "x", "config/a": "y"}
	benign := []Instruction{{Op: OpRead, Text: "config/a"}, {Op: OpEmit, Text: "mentions secret/lab in text"}, {Op: OpWait, Ticks: 3}, {Op: OpEmit, Text: "done"}, {Op: OpHalt}}
	if g := detect(t, Execute(benign, caps), Policy{DelayTicks: 10}); len(g) != 0 {
		t.Fatal("benign program flagged", g)
	}
	heart := []Instruction{{Op: OpEmit, Text: "ping"}, {Op: OpWait, Ticks: 30}, {Op: OpEmit, Text: "ping"}, {Op: OpWait, Ticks: 30}, {Op: OpEmit, Text: "ping"}, {Op: OpHalt}}
	if g := detect(t, Execute(heart, caps), Policy{}); !reflect.DeepEqual(g, []Finding{F("periodic-beacon", 0)}) {
		t.Fatal("heartbeat look-alike not reported without policy", g)
	}
	if g := detect(t, Execute(heart, caps), Policy{Heartbeats: map[string]bool{"ping": true}}); len(g) != 0 {
		t.Fatal(g)
	}
	staged := []Instruction{{Op: OpWait, Ticks: 600}, {Op: OpRead, Text: "secret/lab"}, {Op: OpHalt}}
	want := []Finding{F("delayed-start", 1), F("secret-read", 1)}
	if g := detect(t, Execute(staged, caps), Policy{DelayTicks: 500, Heartbeats: map[string]bool{"ping": true}}); !reflect.DeepEqual(g, want) {
		t.Fatal(g)
	}
	// Jittered beacon: a known false negative of the equal-gap rule.
	jit := []Instruction{{Op: OpRead, Text: "config/a"}, {Op: OpEmit, Text: "b"}, {Op: OpWait, Ticks: 30}, {Op: OpEmit, Text: "b"}, {Op: OpWait, Ticks: 33}, {Op: OpEmit, Text: "b"}, {Op: OpWait, Ticks: 28}, {Op: OpEmit, Text: "b"}, {Op: OpHalt}}
	if g := detect(t, Execute(jit, caps), Policy{DelayTicks: 10}); len(g) != 0 {
		t.Fatal("jitter should evade this rule", g)
	}
}

func TestParseReportVersions(t *testing.T) {
	good := map[string][]Finding{
		`{"schema":1,"findings":[{"id":"delayed-start","index":2},{"id":"secret-read","index":0}]}`: {F("delayed-start", 2), F("secret-read", 0)},
		`{"schema":2,"findings":[{"id":"periodic-beacon","index":4,"severity":"medium"}]}`:          {F("periodic-beacon", 4)},
		` {"schema":2,"findings":[]} `: {},
		`{"findings":[{"severity":"high","index":1,"id":"secret-read"}],"schema":2}`: {F("secret-read", 1)},
		`{"schema":1,"findings":[{"id":"periodic-beacon","index":0}]}` + "\n":        {F("periodic-beacon", 0)},
	}
	for in, want := range good {
		got, err := ParseReport([]byte(in))
		if err != nil || got == nil || !reflect.DeepEqual(got, want) {
			t.Fatal(in, got, err)
		}
	}
	bad := []string{
		``, `null`, `[]`, `{}`, `{"schema":1}`, `{"findings":[]}`, `{"schema":0,"findings":[]}`, `{"schema":3,"findings":[]}`, `{"schema":2.5,"findings":[]}`,
		`{"schema":1,"findings":[{"id":"secret-read","index":0,"severity":"high"}]}`,
		`{"schema":2,"findings":[{"id":"secret-read","index":0}]}`,
		`{"schema":2,"findings":[{"id":"secret-read","index":0,"severity":"low"}]}`,
		`{"schema":2,"findings":[{"id":"secret-read","index":0,"severity":"HIGH"}]}`,
		`{"schema":1,"findings":[{"id":"exec","index":0}]}`,
		`{"schema":1,"findings":[{"id":"secret-read","index":-1}]}`,
		`{"schema":1,"findings":[{"id":"secret-read","index":1.5}]}`,
		`{"schema":1,"findings":[{"id":"secret-read"}]}`[:0] + `{"schema":1,"findings":[{"id":"secret-read","index":0,"extra":1}]}`,
		`{"schema":1,"findings":[],"note":"x"}`,
		`{"schema":1,"findings":[]} {"schema":1,"findings":[]}`,
		`{"schema":1,"findings":[]}x`, `{"schema":1,"findings":[]`, `{"schema":1,"findings":null}`,
		`{"schema":1,"findings":[{"id":"secret-read","index":0}` + strings.Repeat(`,{"id":"secret-read","index":0}`, 64) + `]}`,
	}
	for _, in := range bad {
		if got, err := ParseReport([]byte(in)); err == nil || got != nil {
			t.Fatalf("accepted %q: %v", in, got)
		}
	}
	sixty := `{"schema":1,"findings":[{"id":"secret-read","index":0}` + strings.Repeat(`,{"id":"secret-read","index":0}`, 63) + `]}`
	if got, err := ParseReport([]byte(sixty)); err != nil || len(got) != 64 {
		t.Fatal("64 findings rejected", err)
	}
}
