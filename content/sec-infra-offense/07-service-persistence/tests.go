package main

import (
	"encoding/json"
	"fmt"
	"strings"
	"testing"
)

type serviceLab struct {
	t                          *testing.T
	port                       int
	admin, name, marker, proof string
	unit                       Unit
	exists                     bool
	hardened, sticky           bool
	boots                      int
	stopped, deleted, proved   bool
}

func (s *serviceLab) Send(r Request) Reply {
	if r.Port != s.port || r.Token != s.admin {
		s.t.Fatal("scope/auth")
	}
	path := "/units/" + s.name
	if r.Path == path {
		switch r.Method {
		case "PUT":
			var u Unit
			if json.Unmarshal([]byte(r.Body), &u) != nil || u != (Unit{Name: s.name, RunAs: "root", Command: "emit " + s.marker, Enabled: true}) {
				s.t.Fatal("unit definition")
			}
			if s.hardened {
				return Reply{Status: 403}
			}
			s.unit = u
			s.exists = true
			return Reply{Status: 201}
		case "GET":
			if !s.exists {
				return Reply{Status: 404}
			}
			b, _ := json.Marshal(s.unit)
			return Reply{Status: 200, Body: string(b)}
		case "DELETE":
			if s.unit.Running {
				s.t.Fatal("must stop before deleting")
			}
			s.exists = false
			s.deleted = true
			return Reply{Status: 204}
		}
	}
	if r.Path == path+"/stop" && r.Method == "POST" {
		s.unit.Running = false
		s.stopped = true
		return Reply{Status: 200}
	}
	if r.Path == "/reboot" && r.Method == "POST" {
		s.boots++
		s.proof = ""
		if s.exists && s.unit.Enabled {
			s.unit.Running = true
			s.proof = strings.TrimPrefix(s.unit.Command, "emit ")
		}
		if s.sticky && s.deleted {
			s.proof = s.marker
		}
		return Reply{Status: 200}
	}
	if r.Path == "/service-proof" && r.Method == "GET" {
		if s.proof == s.marker {
			s.proved = true
		}
		return Reply{Status: 200, Body: s.proof}
	}
	s.t.Fatal("unexpected request", r)
	return Reply{}
}
func TestServicePersistence(t *testing.T) {
	for i := 0; i < 12; i++ {
		for _, hard := range []bool{false, true} {
			s := &serviceLab{t: t, port: 12000 + i, admin: fmt.Sprint("operator-", i), name: fmt.Sprint("lab-unit-", i), marker: fmt.Sprint("LAB-PROOF:boot-", i), hardened: hard}
			got, ok := ExerciseService(s, s.port, s.admin, s.name, s.marker)
			if ok == hard || ok && (got.Port != s.port || got.Evidence != s.marker || !s.proved || !s.stopped || !s.deleted || s.boots != 2) {
				t.Fatal(got, ok, hard)
			}
			if s.exists || s.unit.Running {
				t.Fatal("service left behind")
			}
		}
	}
}
func TestServiceCleanupEffect(t *testing.T) {
	s := &serviceLab{t: t, port: 12, admin: "admin", name: "lab-unit", marker: "LAB-PROOF:boot", sticky: true}
	got, ok := ExerciseService(s, 12, "admin", s.name, s.marker)
	if ok || got != (Finding{}) || !s.deleted || s.boots != 2 {
		t.Fatal("post-cleanup persistence missed", got, ok)
	}
}

type serviceProofFailure struct{ inner *serviceLab }

func (s serviceProofFailure) Send(r Request) Reply {
	if r.Path == "/service-proof" && s.inner.exists {
		return Reply{Status: 503}
	}
	return s.inner.Send(r)
}
func TestServiceCleanupAfterProofFailure(t *testing.T) {
	s := &serviceLab{t: t, port: 12, admin: "admin", name: "lab-unit", marker: "LAB-PROOF:boot"}
	got, ok := ExerciseService(serviceProofFailure{s}, 12, "admin", s.name, s.marker)
	if ok || got != (Finding{}) || s.exists || s.unit.Running || !s.stopped || !s.deleted || s.boots != 2 {
		t.Fatal("unconfirmed service must still be removed", got, ok)
	}
}
