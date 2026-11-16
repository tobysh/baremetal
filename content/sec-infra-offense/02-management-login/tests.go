package main

import (
	"encoding/json"
	"fmt"
	"reflect"
	"testing"
)

type loginLab struct {
	t                        *testing.T
	port                     int
	user, key, token, marker string
	allow                    bool
	tried                    map[string]bool
	proof                    bool
}

func (s *loginLab) Send(r Request) Reply {
	if r.Port != s.port {
		s.t.Fatal("scope")
	}
	switch r.Path {
	case "/login":
		if r.Method != "POST" || r.Token != "" {
			s.t.Fatal("login request")
		}
		var b map[string]string
		if json.Unmarshal([]byte(r.Body), &b) != nil || b["user"] != s.user {
			s.t.Fatal("invalid login JSON")
		}
		p := b["password"]
		if p != "wrong" && p != "decoy" && p != s.key {
			s.t.Fatal("unapproved credential")
		}
		if s.tried[p] {
			s.t.Fatal("duplicate attempt")
		}
		s.tried[p] = true
		if p == "decoy" {
			return Reply{Status: 200, Token: "reader"}
		}
		if p == s.key {
			return Reply{Status: 200, Token: s.token}
		}
		return Reply{Status: 401}
	case "/admin/proof":
		if r.Method != "GET" || r.Body != "" {
			s.t.Fatal("proof request")
		}
		if r.Token == s.token && s.allow {
			s.proof = true
			return Reply{Status: 200, Body: s.marker}
		}
		return Reply{Status: 403}
	}
	s.t.Fatal("unexpected path", r.Path)
	return Reply{}
}
func TestManagementLogin(t *testing.T) {
	for i := 0; i < 16; i++ {
		for _, allow := range []bool{true, false} {
			s := &loginLab{t: t, port: 7000 + i, user: fmt.Sprint("operator", i), key: fmt.Sprint("lab-default-", i), token: fmt.Sprint("session-", i), marker: fmt.Sprint("LAB-PROOF:admin-", i), allow: allow, tried: map[string]bool{}}
			keys := []string{"wrong", "decoy", s.key}
			if i%2 == 1 {
				keys = []string{s.key, "decoy", "wrong"}
			}
			got, ok := GuessManagement(s, s.port, s.user, keys)
			if ok != allow || ok && (got.Evidence != s.marker || got.Port != s.port || !s.proof) {
				t.Fatal(got, ok, allow)
			}
		}
	}
}
func TestNoManagementCandidates(t *testing.T) {
	s := &loginLab{t: t}
	got, ok := GuessManagement(s, 0, "", nil)
	if ok || got != (Finding{}) {
		t.Fatal(got, ok)
	}
}

// scriptedLogin answers logins from a table so tests can shape every kind of reply.
type scriptedLogin struct {
	t        *testing.T
	port     int
	user     string
	accounts map[string]Reply // password -> login reply
	proofs   map[string]Reply // token -> /admin/proof reply
	attempts []string         // passwords received, in order
	proofReq []string         // tokens used on /admin/proof, in order
	bodies   []string
}

func (s *scriptedLogin) Send(r Request) Reply {
	if r.Port != s.port {
		s.t.Fatal("scope", r.Port)
	}
	switch r.Path {
	case "/login":
		var b map[string]string
		if r.Method != "POST" || r.Token != "" || json.Unmarshal([]byte(r.Body), &b) != nil || len(b) != 2 || b["user"] != s.user {
			s.t.Fatalf("bad login request: %+v", r)
		}
		s.bodies = append(s.bodies, r.Body)
		s.attempts = append(s.attempts, b["password"])
		if reply, ok := s.accounts[b["password"]]; ok {
			return reply
		}
		s.t.Fatalf("password %q was not in the permitted list", b["password"])
	case "/admin/proof":
		if r.Method != "GET" || r.Body != "" {
			s.t.Fatal("proof request shape")
		}
		s.proofReq = append(s.proofReq, r.Token)
		if reply, ok := s.proofs[r.Token]; ok {
			return reply
		}
		return Reply{Status: 403}
	}
	s.t.Fatal("unexpected path", r.Path)
	return Reply{}
}

func TestLoginBodyIsValidJSONForAwkwardPasswords(t *testing.T) {
	pws := []string{`pa"ss`, `back\slash`, "new\nline", `{"x":1}`, "ünï"}
	acc := map[string]Reply{}
	for _, p := range pws {
		acc[p] = Reply{Status: 401}
	}
	s := &scriptedLogin{t: t, port: 7100, user: "op", accounts: acc, proofs: map[string]Reply{}}
	if got, ok := GuessManagement(s, 7100, "op", pws); ok || got != (Finding{}) {
		t.Fatal(got, ok)
	}
	if !reflect.DeepEqual(s.attempts, pws) {
		t.Fatalf("passwords reached the lab as %q, want %q (is the JSON built with encoding/json?)", s.attempts, pws)
	}
}
func TestStopsAfterFirstConfirmedProof(t *testing.T) {
	s := &scriptedLogin{t: t, port: 7200, user: "op",
		accounts: map[string]Reply{"a": {Status: 401}, "b": {Status: 200, Token: "good"}, "c": {Status: 200, Token: "also-good"}},
		proofs:   map[string]Reply{"good": {Status: 200, Body: "LAB-PROOF:one"}, "also-good": {Status: 200, Body: "LAB-PROOF:two"}}}
	got, ok := GuessManagement(s, 7200, "op", []string{"a", "b", "c"})
	if !ok || got != (Finding{7200, "LAB-PROOF:one"}) {
		t.Fatalf("got %+v %v", got, ok)
	}
	if !reflect.DeepEqual(s.attempts, []string{"a", "b"}) {
		t.Fatalf("attempts %q: should stop after the first confirmed proof", s.attempts)
	}
}
func TestLowRoleSessionIsNotAFinding(t *testing.T) {
	s := &scriptedLogin{t: t, port: 7300, user: "op",
		accounts: map[string]Reply{"x": {Status: 200, Token: "reader"}, "y": {Status: 200, Token: "also-reader"}},
		proofs:   map[string]Reply{}}
	if got, ok := GuessManagement(s, 7300, "op", []string{"x", "y"}); ok || got != (Finding{}) {
		t.Fatalf("low-privilege session reported as compromise: %+v", got)
	}
	if !reflect.DeepEqual(s.proofReq, []string{"reader", "also-reader"}) {
		t.Fatalf("each candidate token must be checked, got %q", s.proofReq)
	}
}
func TestSuccessStatusWithoutTokenIsNotACandidate(t *testing.T) {
	s := &scriptedLogin{t: t, port: 7400, user: "op",
		accounts: map[string]Reply{"x": {Status: 200}, "y": {Status: 204, Token: "t"}, "z": {Status: 200, Token: "real"}},
		proofs:   map[string]Reply{"real": {Status: 200, Body: "LAB-PROOF:late"}, "t": {Status: 200, Body: "LAB-PROOF:wrong-status"}}}
	got, ok := GuessManagement(s, 7400, "op", []string{"x", "y", "z"})
	if !ok || got.Evidence != "LAB-PROOF:late" {
		t.Fatalf("got %+v %v", got, ok)
	}
	if !reflect.DeepEqual(s.proofReq, []string{"real"}) {
		t.Fatalf("proof requested for non-candidates: %q", s.proofReq)
	}
}
func TestProofBodyMustCarryTheMarker(t *testing.T) {
	s := &scriptedLogin{t: t, port: 7500, user: "op",
		accounts: map[string]Reply{"x": {Status: 200, Token: "t1"}, "y": {Status: 200, Token: "t2"}},
		proofs:   map[string]Reply{"t1": {Status: 200, Body: "please log in"}, "t2": {Status: 200, Body: ""}}}
	if got, ok := GuessManagement(s, 7500, "op", []string{"x", "y"}); ok || got != (Finding{}) {
		t.Fatalf("a 200 page without a marker was accepted: %+v", got)
	}
}
func TestEachPasswordTriedOnceAndInOrder(t *testing.T) {
	pws := []string{"p1", "p2", "p3", "p4"}
	acc := map[string]Reply{}
	for _, p := range pws {
		acc[p] = Reply{Status: 401}
	}
	s := &scriptedLogin{t: t, port: 7600, user: "op", accounts: acc, proofs: map[string]Reply{}}
	GuessManagement(s, 7600, "op", pws)
	if !reflect.DeepEqual(s.attempts, pws) {
		t.Fatalf("attempts %q, want exactly %q", s.attempts, pws)
	}
	if len(s.proofReq) != 0 {
		t.Fatal("asked for proof without a session")
	}
}
