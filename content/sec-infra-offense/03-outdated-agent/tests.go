package main

import (
	"encoding/json"
	"fmt"
	"strings"
	"testing"
)

type agentLab struct {
	t               *testing.T
	port            int
	banner, emitted string
	patched         bool
	submitted       bool
	verified        bool
}

func (s *agentLab) Send(r Request) Reply {
	if r.Port != s.port || r.Token != "" {
		s.t.Fatal("scope")
	}
	switch r.Path {
	case "/version":
		if r.Method != "GET" {
			s.t.Fatal("method")
		}
		return Reply{Status: 200, Body: s.banner}
	case "/job":
		if r.Method != "POST" {
			s.t.Fatal("method")
		}
		var b map[string]string
		if json.Unmarshal([]byte(r.Body), &b) != nil {
			s.t.Fatal("job JSON")
		}
		s.submitted = true
		if !s.patched {
			for _, c := range strings.Split("backup "+b["label"], ";") {
				c = strings.TrimSpace(c)
				if strings.HasPrefix(c, "emit ") {
					s.emitted = strings.TrimPrefix(c, "emit ")
				}
			}
		}
		return Reply{Status: 202}
	case "/last-proof":
		if r.Method != "GET" {
			s.t.Fatal("method")
		}
		s.verified = true
		return Reply{Status: 200, Body: s.emitted}
	}
	s.t.Fatal("unexpected operation")
	return Reply{}
}
func TestAgentImpact(t *testing.T) {
	for i := 0; i < 12; i++ {
		for _, patched := range []bool{false, true} {
			s := &agentLab{t: t, port: 8000 + i, banner: "labagent/0.9", patched: patched}
			marker := fmt.Sprint("LAB-PROOF:execution-", i)
			got, ok := ExploitAgent(s, s.port, marker)
			if ok == patched || ok && (got.Evidence != marker || got.Port != s.port || s.emitted != marker) {
				t.Fatal(got, ok, patched)
			}
			if !s.submitted || !s.verified {
				t.Fatal("payload and independent proof read required")
			}
		}
	}
}
func TestNewAgentIsNotAttacked(t *testing.T) {
	s := &agentLab{t: t, port: 8, banner: "labagent/1.0", patched: true}
	got, ok := ExploitAgent(s, 8, "LAB-PROOF:x")
	if ok || got != (Finding{}) || s.submitted {
		t.Fatal("version precondition ignored")
	}
}

// scriptedAgent lets a test choose the version text, the job reply and the stored proof independently.
type scriptedAgent struct {
	t         *testing.T
	port      int
	banner    string
	vuln      bool
	jobStatus int
	stale     string // proof already in memory before the test starts
	proofCode int
	suffix    string // appended to the proof body when read
	jobs      []string
	reads     int
}

func (s *scriptedAgent) Send(r Request) Reply {
	if r.Port != s.port || r.Token != "" {
		s.t.Fatal("scope", r.Port)
	}
	switch r.Path {
	case "/version":
		return Reply{Status: 200, Body: s.banner}
	case "/job":
		var b map[string]string
		if r.Method != "POST" || json.Unmarshal([]byte(r.Body), &b) != nil || len(b) != 1 {
			s.t.Fatalf("job body must be a JSON object with only a label: %q", r.Body)
		}
		s.jobs = append(s.jobs, b["label"])
		if s.vuln {
			for _, c := range strings.Split("backup "+b["label"], ";") {
				if c = strings.TrimSpace(c); strings.HasPrefix(c, "emit ") {
					s.stale = strings.TrimPrefix(c, "emit ")
				}
			}
		}
		return Reply{Status: s.jobStatus}
	case "/last-proof":
		s.reads++
		code := s.proofCode
		if code == 0 {
			code = 200
		}
		return Reply{Status: code, Body: s.stale + s.suffix}
	}
	s.t.Fatal("unexpected path", r.Path)
	return Reply{}
}

func TestVersionPreconditionVariants(t *testing.T) {
	for _, banner := range []string{"labagent/1.0", "labagent/0.9.1", "labagent/0.8", "LABAGENT/0.9", "", "labagent/0.9 "} {
		s := &scriptedAgent{t: t, port: 8100, banner: banner, vuln: true, jobStatus: 202}
		got, ok := ExploitAgent(s, 8100, "LAB-PROOF:pre")
		if ok || got != (Finding{}) || len(s.jobs) != 0 || s.reads != 0 {
			t.Fatalf("banner %q: sent traffic or reported success (%+v, jobs=%d)", banner, got, len(s.jobs))
		}
	}
}
func TestExactlyOneJobWithTheMarkerInIt(t *testing.T) {
	for i, marker := range []string{"LAB-PROOF:a", "LAB-PROOF:run-77", "LAB-PROOF:x-y-z:9"} {
		s := &scriptedAgent{t: t, port: 8200 + i, banner: "labagent/0.9", vuln: true, jobStatus: 202}
		got, ok := ExploitAgent(s, s.port, marker)
		if !ok || got != (Finding{s.port, marker}) {
			t.Fatalf("%s: got %+v %v", marker, got, ok)
		}
		if len(s.jobs) != 1 || !strings.Contains(s.jobs[0], marker) {
			t.Fatalf("expected one job carrying the marker, got %q", s.jobs)
		}
	}
}
func TestJobReplyStatusIsNotEvidence(t *testing.T) {
	for _, status := range []int{200, 202, 500} {
		s := &scriptedAgent{t: t, port: 8300, banner: "labagent/0.9", vuln: true, jobStatus: status}
		got, ok := ExploitAgent(s, 8300, "LAB-PROOF:status")
		if !ok || got.Evidence != "LAB-PROOF:status" || s.reads != 1 {
			t.Fatalf("job status %d changed the outcome: %+v %v reads=%d", status, got, ok, s.reads)
		}
	}
	p := &scriptedAgent{t: t, port: 8301, banner: "labagent/0.9", vuln: false, jobStatus: 202}
	if got, ok := ExploitAgent(p, 8301, "LAB-PROOF:status"); ok || got != (Finding{}) {
		t.Fatalf("patched target reported as exploited: %+v", got)
	}
}
func TestStaleProofFromEarlierRunIsNotEvidence(t *testing.T) {
	s := &scriptedAgent{t: t, port: 8400, banner: "labagent/0.9", vuln: false, jobStatus: 202, stale: "LAB-PROOF:someone-else"}
	if got, ok := ExploitAgent(s, 8400, "LAB-PROOF:mine"); ok || got != (Finding{}) {
		t.Fatalf("accepted a proof that is not the supplied marker: %+v", got)
	}
}
func TestProofMustMatchExactlyAndWithStatus200(t *testing.T) {
	cases := []*scriptedAgent{
		{banner: "labagent/0.9", vuln: true, jobStatus: 202, suffix: "-extra"},
		{banner: "labagent/0.9", vuln: true, jobStatus: 202, proofCode: 404},
		{banner: "labagent/0.9", vuln: true, jobStatus: 202, proofCode: 500},
	}
	for i, s := range cases {
		s.t, s.port = t, 8500+i
		if got, ok := ExploitAgent(s, s.port, "LAB-PROOF:exact"); ok || got != (Finding{}) {
			t.Fatalf("case %d accepted a body or status that is not exactly the marker: %+v", i, got)
		}
	}
}
func TestPayloadSurvivesJSONRoundTrip(t *testing.T) {
	s := &scriptedAgent{t: t, port: 8600, banner: "labagent/0.9", vuln: true, jobStatus: 202}
	if got, ok := ExploitAgent(s, 8600, "LAB-PROOF:json"); !ok || got.Port != 8600 {
		t.Fatal(got, ok)
	}
	if len(s.jobs) != 1 || !strings.Contains(s.jobs[0], ";") {
		t.Fatalf("the label %q does not end the first command with a semicolon", s.jobs)
	}
}
