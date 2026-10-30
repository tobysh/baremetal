package main

import (
	"encoding/json"
	"fmt"
	"testing"
)

type rotationLab struct {
	t                                           *testing.T
	backup, manager                             int
	key, old, new, marker, mode                 string
	leaked, proved, rotated, deleted, attempted bool
	step                                        int
}

func (s *rotationLab) Send(r Request) Reply {
	if r.Port != s.backup && r.Port != s.manager {
		s.t.Fatal("out-of-scope request")
	}
	if r.Body != "" {
		s.t.Fatal("unexpected body")
	}
	if r.Path == "/banner" && r.Method == "GET" && r.Token == "" {
		if r.Port == s.backup {
			return Reply{Status: 200, Body: "labbackup/0.8"}
		}
		return Reply{Status: 200, Body: "manager"}
	}
	if r.Port == s.backup && r.Path == "/public/config" && r.Method == "GET" && r.Token == "" {
		b, _ := json.Marshal(LeakConfig{s.manager, s.key})
		return Reply{Status: 200, Body: string(b)}
	}
	if r.Port == s.backup && r.Path == "/public/value/"+s.key {
		if r.Method == "DELETE" {
			if !s.rotated || r.Token != s.new || s.step != 3 {
				s.t.Fatal("repair order/credential")
			}
			s.attempted = true
			s.step = 4
			if s.mode != "delete-lies" {
				s.deleted = true
			}
			return Reply{Status: 204}
		}
		if r.Method == "GET" && r.Token == "" {
			if s.rotated {
				if s.step != 4 {
					s.t.Fatal("verify removal after deletion")
				}
				s.step = 5
			}
			if s.deleted {
				return Reply{Status: 404}
			}
			s.leaked = true
			b, _ := json.Marshal(LeakValue{s.old})
			return Reply{Status: 200, Body: string(b)}
		}
	}
	if r.Port == s.manager && r.Path == "/admin/rotate" && r.Method == "POST" {
		if !s.proved || r.Token != s.old {
			s.t.Fatal("unconfirmed rotation")
		}
		if s.mode == "rotation-fails" {
			return Reply{Status: 500}
		}
		s.rotated = true
		s.step = 1
		return Reply{Status: 200, Token: s.new}
	}
	if r.Port == s.manager && r.Path == "/admin/snapshot" && r.Method == "GET" {
		if !s.leaked {
			s.t.Fatal("no leaked credential")
		}
		if !s.rotated {
			if r.Token != s.old {
				s.t.Fatal("wrong initial token")
			}
			s.proved = true
			return Reply{Status: 200, Body: s.marker}
		}
		if s.step == 1 && r.Token == s.old {
			s.step = 2
			if s.mode == "old-still-valid" {
				return Reply{Status: 200, Body: s.marker}
			}
			if s.mode == "old-200-decoy" {
				return Reply{Status: 200, Body: "login required"}
			}
			return Reply{Status: 401}
		}
		if s.step == 2 && r.Token == s.new {
			s.step = 3
			if s.mode == "new-broken" {
				return Reply{Status: 403}
			}
			if s.mode == "new-wrong-proof" {
				return Reply{Status: 200, Body: "LAB-PROOF:different"}
			}
			return Reply{Status: 200, Body: s.marker}
		}
	}
	s.t.Fatal("unexpected request/order", r, s.step)
	return Reply{}
}
func TestRotationLifecycle(t *testing.T) {
	for i := 0; i < 10; i++ {
		for _, mode := range []string{"ok", "rotation-fails", "old-still-valid", "old-200-decoy", "new-broken", "new-wrong-proof", "delete-lies"} {
			s := &rotationLab{t: t, backup: 15000 + i, manager: 16000 + i, key: fmt.Sprint("key-", i), old: fmt.Sprint("old-", i), new: fmt.Sprint("new-", i), marker: fmt.Sprint("LAB-PROOF:chain-", i), mode: mode}
			got, ok := ChainAndRotate(s, []int{s.manager, s.backup})
			if ok != (mode == "ok") || ok && (got.Port != s.manager || got.Evidence != s.marker || !s.deleted || s.step != 5) {
				t.Fatal(mode, got, ok)
			}
			if s.rotated && (!s.attempted || s.step != 5) {
				t.Fatal("repair abandoned after verification failure", mode)
			}
		}
	}
}
func TestEmptyRotationScope(t *testing.T) {
	s := &rotationLab{t: t}
	got, ok := ChainAndRotate(s, nil)
	if ok || got != (Finding{}) {
		t.Fatal(got, ok)
	}
}
