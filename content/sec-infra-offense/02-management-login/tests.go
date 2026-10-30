package main

import (
	"encoding/json"
	"fmt"
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
