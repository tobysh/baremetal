package main

import (
	"encoding/json"
	"fmt"
	"strings"
	"testing"
)

type accountLab struct {
	t                      *testing.T
	port                   int
	admin, marker, session string
	want                   Account
	created                bool
	hardened, sticky       bool
	proved, deleted        bool
}

func (s *accountLab) Send(r Request) Reply {
	if r.Port != s.port {
		s.t.Fatal("scope")
	}
	switch r.Path {
	case "/accounts":
		if r.Method != "POST" || r.Token != s.admin {
			s.t.Fatal("create auth/method")
		}
		var a Account
		if json.Unmarshal([]byte(r.Body), &a) != nil || a != s.want {
			s.t.Fatal("account payload")
		}
		if s.hardened {
			return Reply{Status: 403}
		}
		s.created = true
		return Reply{Status: 201}
	case "/accounts/all", "/accounts/visible":
		if r.Method != "GET" || r.Token != s.admin {
			s.t.Fatal("audit auth/method")
		}
		items := []string{"operator"}
		if s.created && (r.Path == "/accounts/all" || !s.want.Hidden) {
			items = append(items, s.want.Name)
		}
		return Reply{Status: 200, Items: items}
	case "/login":
		if r.Method != "POST" || r.Token != "" {
			s.t.Fatal("fresh login required")
		}
		var b map[string]string
		if json.Unmarshal([]byte(r.Body), &b) != nil || b["user"] != s.want.Name || b["password"] != s.want.Password {
			s.t.Fatal("credentials")
		}
		if s.created {
			return Reply{Status: 200, Token: s.session}
		}
		return Reply{Status: 401}
	case "/root/proof":
		if r.Method != "GET" || r.Token != s.session {
			s.t.Fatal("use fresh account token")
		}
		if s.created || s.sticky && s.deleted {
			s.proved = true
			return Reply{Status: 200, Body: s.marker}
		}
		return Reply{Status: 401}
	}
	if r.Method == "DELETE" && r.Path == "/accounts/"+s.want.Name && r.Token == s.admin {
		s.created = false
		s.deleted = true
		return Reply{Status: 204}
	}
	if strings.HasPrefix(r.Path, "/accounts/") {
		s.t.Fatal("unexpected account operation")
	}
	s.t.Fatal("unexpected path")
	return Reply{}
}
func TestAccountPersistenceAndCleanup(t *testing.T) {
	for i := 0; i < 12; i++ {
		for _, hard := range []bool{false, true} {
			s := &accountLab{t: t, port: 11000 + i, admin: fmt.Sprint("admin-", i), session: fmt.Sprint("new-session-", i), marker: fmt.Sprint("LAB-PROOF:account-", i), want: Account{fmt.Sprint("lab-audit-", i), fmt.Sprint("lab-key-", i), 0, true}, hardened: hard}
			got, ok := ExerciseAccount(s, s.port, s.admin, s.want)
			if ok == hard || ok && (got.Evidence != s.marker || got.Port != s.port || !s.proved || !s.deleted) {
				t.Fatal(got, ok, hard)
			}
			if s.created {
				t.Fatal("account left behind")
			}
		}
	}
}
func TestDeletionMustRevokeSession(t *testing.T) {
	s := &accountLab{t: t, port: 11, admin: "admin", session: "new", marker: "LAB-PROOF:account", want: Account{"lab-audit", "key", 0, true}, sticky: true}
	got, ok := ExerciseAccount(s, 11, "admin", s.want)
	if ok || got != (Finding{}) || !s.deleted {
		t.Fatal("session revocation not verified", got, ok)
	}
}

type accountProofFailure struct{ inner *accountLab }

func (s accountProofFailure) Send(r Request) Reply {
	if r.Path == "/root/proof" && s.inner.created {
		return Reply{Status: 403}
	}
	return s.inner.Send(r)
}
func TestAccountCleanupAfterProofFailure(t *testing.T) {
	s := &accountLab{t: t, port: 11, admin: "admin", session: "new", marker: "LAB-PROOF:account", want: Account{"lab-audit", "key", 0, true}}
	got, ok := ExerciseAccount(accountProofFailure{s}, 11, "admin", s.want)
	if ok || got != (Finding{}) || s.created || !s.deleted {
		t.Fatal("unconfirmed account must still be removed", got, ok)
	}
}
