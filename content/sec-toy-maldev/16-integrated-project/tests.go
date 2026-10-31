package main

import (
	"encoding/binary"
	"fmt"
	"hash/crc32"
	"reflect"
	"strings"
	"testing"
)

// Independent fixtures, written without the learner's helpers.
func pjCapsule(code []byte, entry uint16) []byte {
	b := make([]byte, 12+len(code))
	copy(b, "BML1")
	binary.LittleEndian.PutUint16(b[4:6], entry)
	binary.LittleEndian.PutUint16(b[6:8], uint16(len(code)))
	binary.LittleEndian.PutUint32(b[8:12], crc32.ChecksumIEEE(code))
	copy(b[12:], code)
	return b
}
func pjPackage(plain []byte, key byte) []byte {
	b := make([]byte, 11+len(plain))
	copy(b, "BX01")
	binary.LittleEndian.PutUint16(b[4:6], uint16(len(plain)))
	b[6] = key
	binary.LittleEndian.PutUint32(b[7:11], crc32.ChecksumIEEE(plain))
	for i, v := range plain {
		b[11+i] = v ^ key
	}
	return b
}
func pjEmit(s string) []byte { return append([]byte{1, byte(len(s))}, s...) }
func pjRead(s string) []byte { return append([]byte{2, byte(len(s))}, s...) }
func pjWait(n uint16) []byte { return []byte{3, byte(n), byte(n >> 8)} }

func pjSteps(prefix string) []Step {
	return []Step{{Op: "emit", Arg: prefix}, {Op: "entry"}, {Op: "emit-sym", Arg: "greeting"}, {Op: "read", Arg: "note"}, {Op: "wait", Ticks: 4}}
}

func TestCheckpoint1Resolve(t *testing.T) {
	cfg := map[string]string{"greeting": "hi\x00"}
	prog, entry, err := Resolve(pjSteps("boot"), cfg)
	want := []Instruction{{Op: OpEmit, Text: "boot"}, {Op: OpEmit, Text: "hi\x00"}, {Op: OpRead, Text: "note"}, {Op: OpWait, Ticks: 4}, {Op: OpHalt}}
	if err != nil || entry != 1 || !reflect.DeepEqual(prog, want) || len(cfg) != 1 || cfg["greeting"] != "hi\x00" {
		t.Fatal(prog, entry, err)
	}
	prog, entry, err = Resolve([]Step{{Op: "read", Arg: "k"}}, nil)
	if err != nil || entry != 0 || len(prog) != 2 {
		t.Fatal("no entry marker means entry 0", prog, entry, err)
	}
	_, entry, err = Resolve([]Step{{Op: "emit", Arg: "a"}, {Op: "entry"}}, nil)
	if err != nil || entry != 1 {
		t.Fatal("entry at the end points at HALT", entry, err)
	}
	long := strings.Repeat("x", 256)
	many := make([]Step, 64)
	for i := range many {
		many[i] = Step{Op: "emit", Arg: "a"}
	}
	bad := []struct {
		steps []Step
		cfg   map[string]string
	}{
		{[]Step{{Op: "emit-sym", Arg: "ghost"}}, nil},
		{[]Step{{Op: "emit", Arg: "a"}}, map[string]string{"typo": "v"}},
		{[]Step{{Op: "jump", Arg: "a"}}, nil},
		{[]Step{{Op: "entry"}, {Op: "entry"}}, nil},
		{[]Step{{Op: "wait"}}, nil},
		{[]Step{{Op: "read"}}, nil},
		{[]Step{{Op: "emit", Arg: long}}, nil},
		{[]Step{{Op: "emit-sym", Arg: "s"}}, map[string]string{"s": long}},
		{many, nil},
	}
	for i, c := range bad {
		if p, e, err := Resolve(c.steps, c.cfg); err == nil || p != nil || e != 0 {
			t.Fatal("bad template accepted", i)
		}
	}
	if _, _, err := Resolve(many[:63], nil); err != nil {
		t.Fatal("63 steps plus HALT is allowed", err)
	}
}

func TestCheckpoint2Build(t *testing.T) {
	for i := 0; i < 8; i++ {
		greeting := fmt.Sprint("hello-", i, "\x00")
		prefix := strings.Repeat("p", i+1)
		code := append(pjEmit(prefix), pjEmit(greeting)...)
		code = append(code, pjRead("note")...)
		code = append(code, pjWait(4)...)
		code = append(code, 0)
		want := pjPackage(pjCapsule(code, uint16(len(pjEmit(prefix)))), byte(i+3))
		got, err := Build(pjSteps(prefix), map[string]string{"greeting": greeting}, byte(i+3))
		if err != nil || !reflect.DeepEqual(got, want) {
			t.Fatal(i, got, want, err)
		}
	}
	for _, c := range []struct {
		steps []Step
		cfg   map[string]string
		key   byte
	}{{pjSteps("a"), nil, 3}, {pjSteps("a"), map[string]string{"greeting": "x"}, 0}, {[]Step{{Op: "emit-sym", Arg: "x"}}, map[string]string{"x": "1", "y": "2"}, 4}} {
		if b, err := Build(c.steps, c.cfg, c.key); err == nil || b != nil {
			t.Fatal("Build must propagate errors")
		}
	}
	// A wait before the entry point has a three-byte encoding.
	got, _ := Build([]Step{{Op: "wait", Ticks: 300}, {Op: "entry"}, {Op: "emit", Arg: "z"}}, nil, 9)
	want := pjPackage(pjCapsule(append(append(pjWait(300), pjEmit("z")...), 0), 3), 9)
	if !reflect.DeepEqual(got, want) {
		t.Fatal("wait offsets", got)
	}
}

func TestCheckpoint3Detect(t *testing.T) {
	rules := Rules{AllowedReads: map[string]bool{"ok": true, "no": false}, MaxTicks: 5, Forbidden: []string{"MARK", "", "zz"}}
	cases := []struct {
		tr   Trace
		want []string
	}{
		{Trace{Outputs: []string{"fine"}, Reads: []string{"ok"}, Ticks: 5}, []string{}},
		{Trace{Reads: []string{"ok", "bad", "no", "bad"}, Ticks: 6}, []string{"read:bad", "read:no", "ticks:6"}},
		{Trace{Outputs: []string{"xxMARKxx", "MARK", "a zz b"}}, []string{"output:MARK", "output:zz"}},
		{Trace{Outputs: []string{"mark"}, Reads: nil, Ticks: 0}, []string{}},
	}
	for i, c := range cases {
		got := Detect(c.tr, rules)
		if got == nil || !reflect.DeepEqual(got, c.want) {
			t.Fatal(i, got)
		}
	}
	if got := Detect(Trace{Reads: []string{"x"}}, Rules{}); !reflect.DeepEqual(got, []string{"read:x"}) {
		t.Fatal("nil allow-list allows nothing", got)
	}
}

func TestCheckpoint4DeployAndTeardown(t *testing.T) {
	h := &Host{}
	var journal []string
	tr := Trace{Outputs: []string{"a", "b"}}
	if err := Deploy(h, &journal, "lab-1", []byte{0, 1, 2}, tr); err != nil {
		t.Fatal(err)
	}
	if h.Files["lab-1/payload.bin"] != "\x00\x01\x02" || h.Files["lab-1/trace.log"] != "a\nb" || !reflect.DeepEqual(journal, []string{"lab-1/payload.bin", "lab-1/trace.log"}) {
		t.Fatal(h.Files, journal)
	}
	Teardown(h, &journal)
	Teardown(h, &journal)
	if len(h.Files) != 0 || len(journal) != 0 {
		t.Fatal("teardown left state", h.Files, journal)
	}
	// A collision on the second file must not delete the pre-existing entry.
	h = &Host{Files: map[string]string{"x/trace.log": "mine", "other": "o"}}
	journal = nil
	if err := Deploy(h, &journal, "x", []byte("b"), tr); err == nil {
		t.Fatal("overwrite accepted")
	}
	if !reflect.DeepEqual(journal, []string{"x/payload.bin"}) {
		t.Fatal("journal must list only what was created", journal)
	}
	Teardown(h, &journal)
	if !reflect.DeepEqual(h.Files, map[string]string{"x/trace.log": "mine", "other": "o"}) {
		t.Fatal("pre-existing state damaged", h.Files)
	}
	for _, n := range []string{"", "UPPER", "a/b", "has space", strings.Repeat("a", 33)} {
		j := []string{}
		hh := &Host{Files: map[string]string{}}
		if err := Deploy(hh, &j, n, nil, tr); err == nil || len(hh.Files) != 0 || len(j) != 0 {
			t.Fatal("bad name accepted", n)
		}
	}
}

func pjBaseline() *Host {
	return &Host{Files: map[string]string{"keep.txt": "mine", "demo/other": "unrelated"}}
}

func TestCheckpoint5RunProject(t *testing.T) {
	rules := Rules{AllowedReads: map[string]bool{"note": true}, MaxTicks: 5, Forbidden: []string{"SECRET"}}
	lim := Limits{MaxOps: 10, MaxTicks: 10}
	caps := map[string]string{"note": "public"}
	good := map[string]string{"greeting": "hello"}
	cases := []struct {
		name     string
		steps    []Step
		cfg      map[string]string
		caps     map[string]string
		lim      Limits
		rules    Rules
		wantErr  bool
		findings []string
		outputs  []string
	}{
		{"clean", pjSteps("pre"), good, caps, lim, rules, false, []string{}, []string{"hello", "public"}},
		{"ticks", pjSteps("pre"), good, caps, lim, Rules{AllowedReads: rules.AllowedReads, MaxTicks: 3}, false, []string{"ticks:4"}, []string{"hello", "public"}},
		{"unexpected read", pjSteps("pre"), good, caps, lim, Rules{MaxTicks: 9}, false, []string{"read:note"}, []string{"hello", "public"}},
		{"marker", pjSteps("pre"), map[string]string{"greeting": "a SECRET b"}, caps, lim, rules, false, []string{"output:SECRET"}, []string{"a SECRET b", "public"}},
		{"unresolved", pjSteps("pre"), nil, caps, lim, rules, true, nil, nil},
		{"missing capability", pjSteps("pre"), good, nil, lim, rules, true, nil, nil},
		{"op budget", pjSteps("pre"), good, caps, Limits{MaxOps: 2, MaxTicks: 10}, rules, true, nil, nil},
		{"tick budget", pjSteps("pre"), good, caps, Limits{MaxOps: 10, MaxTicks: 1}, rules, true, nil, nil},
	}
	for _, c := range cases {
		h := pjBaseline()
		capsBefore := fmt.Sprint(c.caps)
		rep, err := RunProject(h, "demo", c.steps, c.cfg, 21, c.caps, c.lim, c.rules)
		if !reflect.DeepEqual(h, pjBaseline()) {
			t.Fatal(c.name, "host not restored", h.Files)
		}
		if fmt.Sprint(c.caps) != capsBefore {
			t.Fatal(c.name, "caps modified")
		}
		if c.wantErr {
			if err == nil || !reflect.DeepEqual(rep, Report{}) {
				t.Fatal(c.name, "expected error", rep, err)
			}
			continue
		}
		if err != nil || !reflect.DeepEqual(rep.Findings, c.findings) || !reflect.DeepEqual(rep.Trace.Outputs, c.outputs) {
			t.Fatal(c.name, rep, err)
		}
	}
}

func TestCheckpoint5CollisionLeavesOtherStateAlone(t *testing.T) {
	h := &Host{Files: map[string]string{"demo/trace.log": "someone else's"}}
	_, err := RunProject(h, "demo", pjSteps("p"), map[string]string{"greeting": "g"}, 5, map[string]string{"note": "n"}, Limits{10, 10}, Rules{})
	if err == nil || !reflect.DeepEqual(h.Files, map[string]string{"demo/trace.log": "someone else's"}) {
		t.Fatal(err, h.Files)
	}
}

func TestEarlierCheckpointsStillHoldAndAgree(t *testing.T) {
	steps := pjSteps("pre")
	cfg := map[string]string{"greeting": "hello"}
	caps := map[string]string{"note": "public"}
	bundle, err := Build(steps, cfg, 21)
	if err != nil {
		t.Fatal(err)
	}
	direct, err := ExecuteBundle(bundle, caps, 10, 10)
	if err != nil || !reflect.DeepEqual(direct.Outputs, []string{"hello", "public"}) {
		t.Fatal("prefix must be skipped, suffix executed", direct, err)
	}
	rules := Rules{AllowedReads: map[string]bool{}, MaxTicks: 1}
	rep, err := RunProject(&Host{}, "agree", steps, cfg, 21, caps, Limits{10, 10}, rules)
	if err != nil || !reflect.DeepEqual(rep.Trace, direct) || !reflect.DeepEqual(rep.Findings, Detect(direct, rules)) {
		t.Fatal("final pipeline disagrees with the checkpoints", rep, err)
	}
	for _, key := range []byte{1, 50, 200} {
		b1, _ := Build(steps, cfg, key)
		tr, err := ExecuteBundle(b1, caps, 10, 10)
		if err != nil || !reflect.DeepEqual(tr, direct) {
			t.Fatal("packaging key changed behavior", key)
		}
	}
}
