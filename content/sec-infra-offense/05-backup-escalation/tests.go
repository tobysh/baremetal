package main

import (
	"encoding/json"
	"fmt"
	"testing"
)

type backupLab struct {
	t                         *testing.T
	port                      int
	token, marker, role       string
	config, original          BackupConfig
	hardened, breakCleanup    bool
	changed, proved, executed bool
}

func (s *backupLab) Send(r Request) Reply {
	if r.Port != s.port || r.Token != s.token {
		s.t.Fatal("scope/token")
	}
	switch r.Path {
	case "/identity":
		if r.Method != "GET" {
			s.t.Fatal("method")
		}
		return Reply{Status: 200, Body: s.role}
	case "/backup/config":
		if r.Method == "GET" {
			b, _ := json.Marshal(s.config)
			return Reply{Status: 200, Body: string(b)}
		}
		if r.Method != "PUT" {
			s.t.Fatal("method")
		}
		var c BackupConfig
		if json.Unmarshal([]byte(r.Body), &c) != nil || c.Path != s.original.Path || c.Owner != s.original.Owner || c.Writable != s.original.Writable || (c.Command != "archive" && c.Command != "grant-root") {
			s.t.Fatal("unexpected config")
		}
		if s.hardened {
			return Reply{Status: 403}
		}
		if s.breakCleanup && s.changed && c.Command == "archive" {
			return Reply{Status: 200}
		}
		s.config = c
		if c.Command == "grant-root" {
			s.changed = true
		}
		return Reply{Status: 200}
	case "/backup/run":
		if r.Method != "POST" {
			s.t.Fatal("method")
		}
		s.executed = true
		if s.config.Command == "grant-root" {
			s.role = "root"
		}
		return Reply{Status: 202}
	case "/root/proof":
		if r.Method != "GET" {
			s.t.Fatal("method")
		}
		if s.role == "root" {
			s.proved = true
			return Reply{Status: 200, Body: s.marker}
		}
		return Reply{Status: 403}
	case "/session/drop-root":
		if r.Method != "POST" {
			s.t.Fatal("method")
		}
		s.role = "user"
		return Reply{Status: 200}
	}
	s.t.Fatal("unexpected path", r.Path)
	return Reply{}
}
func TestBackupPrivilegeAndCleanup(t *testing.T) {
	for i := 0; i < 12; i++ {
		for _, hard := range []bool{false, true} {
			c := BackupConfig{Path: fmt.Sprint("/opt/job-", i), Owner: "root", Writable: true, Command: "archive"}
			s := &backupLab{t: t, port: 10000 + i, token: fmt.Sprint("low-", i), marker: fmt.Sprint("LAB-PROOF:root-", i), role: "user", config: c, original: c, hardened: hard}
			got, ok := ExerciseBackup(s, s.port, s.token)
			if ok == hard || ok && (got.Port != s.port || got.Evidence != s.marker || !s.proved || !s.executed) {
				t.Fatal(got, ok, hard)
			}
			if s.config != c || s.role != "user" {
				t.Fatal("cleanup missing", s.config, s.role)
			}
		}
	}
}
func TestBackupCleanupMustBeVerified(t *testing.T) {
	c := BackupConfig{Path: "/opt/job", Owner: "root", Writable: true, Command: "archive"}
	s := &backupLab{t: t, port: 10, token: "low", marker: "LAB-PROOF:root", role: "user", config: c, original: c, breakCleanup: true}
	got, ok := ExerciseBackup(s, 10, "low")
	if ok || got != (Finding{}) || !s.proved {
		t.Fatal("unverified cleanup accepted", got, ok)
	}
}

// Exploitation can change state even when the evidence endpoint fails.
type backupProofFailure struct{ inner *backupLab }

func (s backupProofFailure) Send(r Request) Reply {
	if r.Path == "/root/proof" {
		return Reply{Status: 503}
	}
	return s.inner.Send(r)
}
func TestBackupCleansAfterProofFailure(t *testing.T) {
	c := BackupConfig{Path: "/opt/job", Owner: "root", Writable: true, Command: "archive"}
	s := &backupLab{t: t, port: 10, token: "low", marker: "LAB-PROOF:root", role: "user", config: c, original: c}
	got, ok := ExerciseBackup(backupProofFailure{s}, 10, "low")
	if ok || got != (Finding{}) || !s.changed || !s.executed || s.role != "user" || s.config != c {
		t.Fatal("failed proof must still restore state", got, ok, s.config, s.role)
	}
}
