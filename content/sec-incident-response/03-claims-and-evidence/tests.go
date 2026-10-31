package main

import (
	"fmt"
	"testing"
)

func TestPublic(t *testing.T) {
	ev := []Evidence{{"E1", "netstat", "web1", "listener", "4444", true}, {"T1", "ticket", "web1", "listener", "4444", false}}
	got := Assess(ev, []Claim{
		{"c1", "web1", "listener", "4444", []string{"E1"}, nil},
		{"c2", "web1", "listener", "4444", []string{"T1"}, nil},
	})
	if got[0].Status != "supported" || got[1].Status != "reported" {
		t.Fatal(got)
	}
}

func TestVariants(t *testing.T) {
	for i := 0; i < 12; i++ {
		h := fmt.Sprint("host", i)
		p := func(s string) string { return fmt.Sprint(s, i) }
		ev := []Evidence{
			{p("O1"), "edr", h, "proc", "nc", true},
			{p("O2"), "fw", h, "out-port", "443", true},
			{p("O3"), "auth", h, "login", "ok", true},
			{p("H1"), "chat", h, "owner", "alice", false},
			{p("H2"), "chat", h, "proc", "bash", false},
			{p("O4"), "edr", h, "parent", "cron", true},
		}
		cases := []struct {
			c    Claim
			want string
		}{
			{Claim{"a", h, "proc", "nc", []string{p("O1")}, nil}, "supported"},
			{Claim{"b", h, "owner", "alice", []string{p("H1")}, nil}, "reported"},
			{Claim{"c", h, "proc", "bash", []string{p("H2")}, nil}, "contradicted"}, // observed nc elsewhere
			{Claim{"d", h, "proc", "nc", []string{p("O2")}, []string{"nc implies tunnel"}}, "supported" /* O1 not cited */},
			{Claim{"e", h, "role", "c2-beacon", []string{p("O2")}, []string{"443 is beacon port"}}, "inferred"},
			{Claim{"f", h, "role", "c2-beacon", []string{p("O2")}, nil}, "unsupported"},
			{Claim{"g", h, "role", "x", nil, []string{"anything"}}, "unsupported"},
			{Claim{"h", h, "login", "ok", []string{p("O3"), "missing"}, nil}, "unsupported"},
			{Claim{"i", h, "parent", "cron", []string{p("O4"), p("H2")}, nil}, "supported"},
			{Claim{"j", h, "parent", "sshd", []string{p("O4")}, []string{"x"}}, "contradicted"},
			{Claim{"k", h, "login", "ok", []string{p("H1")}, []string{"alice logged in"}}, "inferred"},
		}
		// claim d is subtle: it cites O2 only, so no direct match; assumption declared -> inferred
		cases[3].want = "inferred"
		var claims []Claim
		for _, c := range cases {
			claims = append(claims, c.c)
		}
		got := Assess(ev, claims)
		if len(got) != len(cases) {
			t.Fatal("verdict count", len(got))
		}
		for k, c := range cases {
			if got[k].ClaimID != c.c.ID || got[k].Status != c.want {
				t.Fatalf("variant %d claim %s: got %+v want %s", i, c.c.ID, got[k], c.want)
			}
		}
	}
}
