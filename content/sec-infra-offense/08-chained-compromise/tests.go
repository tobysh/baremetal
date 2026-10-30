package main

import (
	"encoding/json"
	"fmt"
	"testing"
)

type chainLab struct {
	t                  *testing.T
	backup, manager    int
	key, token, marker string
	mode               string
	leaked, proved     bool
}

func (s *chainLab) Send(r Request) Reply {
	if r.Port != s.backup && r.Port != s.manager && r.Port != s.backup+1 {
		s.t.Fatal("out-of-scope pivot", r.Port)
	}
	if r.Method != "GET" || r.Body != "" {
		s.t.Fatal("unexpected mutation")
	}
	if r.Path == "/banner" {
		if r.Token != "" {
			s.t.Fatal("banner token")
		}
		if r.Port == s.backup {
			return Reply{Status: 200, Body: "labbackup/0.8"}
		}
		return Reply{Status: 200, Body: "labother/1.0"}
	}
	if r.Port == s.backup {
		if r.Token != "" {
			s.t.Fatal("public read token")
		}
		switch r.Path {
		case "/public/config":
			if s.mode == "config-closed" {
				return Reply{Status: 403}
			}
			if s.mode == "bad-json" {
				return Reply{Status: 200, Body: "{"}
			}
			cfg := LeakConfig{s.manager, s.key}
			if s.mode == "outside" {
				cfg.ManagementPort = 65000
			}
			b, _ := json.Marshal(cfg)
			return Reply{Status: 200, Body: string(b)}
		case "/public/value/" + s.key:
			if s.mode == "value-closed" {
				return Reply{Status: 403}
			}
			s.leaked = true
			b, _ := json.Marshal(LeakValue{s.token})
			return Reply{Status: 200, Body: string(b)}
		}
	}
	if r.Port == s.manager && r.Path == "/admin/snapshot" {
		if !s.leaked || r.Token != s.token {
			s.t.Fatal("token must come from leak")
		}
		if s.mode == "denied" {
			return Reply{Status: 403}
		}
		if s.mode == "decoy" {
			return Reply{Status: 200, Body: "login required"}
		}
		s.proved = true
		return Reply{Status: 200, Body: s.marker}
	}
	s.t.Fatal("unexpected request", r)
	return Reply{}
}
func TestChainVariants(t *testing.T) {
	for i := 0; i < 10; i++ {
		for _, mode := range []string{"vulnerable", "config-closed", "value-closed", "outside", "bad-json", "denied", "decoy"} {
			s := &chainLab{t: t, backup: 13000 + i*3, manager: 14000 + i*5, key: fmt.Sprint("management-key-", i), token: fmt.Sprint("leaked-token-", i), marker: fmt.Sprint("LAB-PROOF:snapshot-", i), mode: mode}
			got, ok := ChainSnapshot(s, []int{s.backup + 1, s.backup, s.manager})
			want := mode == "vulnerable"
			if ok != want || ok && (got.Port != s.manager || got.Evidence != s.marker || !s.proved) {
				t.Fatal(mode, got, ok)
			}
		}
	}
}
func TestEmptyChainScope(t *testing.T) {
	s := &chainLab{t: t}
	got, ok := ChainSnapshot(s, nil)
	if ok || got != (Finding{}) {
		t.Fatal(got, ok)
	}
}
