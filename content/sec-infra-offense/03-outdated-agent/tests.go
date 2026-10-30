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
