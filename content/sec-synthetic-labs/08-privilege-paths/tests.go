package main

import (
	"encoding/json"
	"fmt"
	"reflect"
	"testing"
)

func TestPrivilegeVariants(t *testing.T) {
	for i := 0; i < 20; i++ {
		p := fmt.Sprintf("/opt/jobs/task%d.sh", i)
		s := State{User: "guest", Groups: []string{"ops"}, Files: []File{{p, 0770, "root", "ops"}, {"/opt/sudo", 0622, "root", "other"}, {"/safe", 0644, "root", "root"}, {"/owned", 0422, "guest", "other"}, {"/group", 0702, "root", "ops"}}, Cron: []Job{{"root", p}, {"root", p}, {"root", "/safe"}, {"root", "/owned"}, {"root", "/group"}, {"root", "/absent"}}, Sudo: []SudoRule{{"guest", "root", "/opt/sudo", true}}}
		b, _ := json.Marshal(s)
		got, err := EscalationPaths(string(b))
		want := []string{p, "/opt/sudo"}
		if err != nil || !reflect.DeepEqual(got, want) {
			t.Fatalf("got %+v %v want %+v", got, err, want)
		}
		s.Cron = nil
		s.Sudo = []SudoRule{{"guest", "root", p, false}, {"other", "root", p, true}, {"guest", "daemon", p, true}}
		b, _ = json.Marshal(s)
		got, err = EscalationPaths(string(b))
		if err != nil || len(got) != 0 {
			t.Fatal("ineligible execution", got, err)
		}
	}
}
func TestPrivilegeInvalid(t *testing.T) {
	if _, err := EscalationPaths("{"); err == nil {
		t.Fatal("invalid JSON accepted")
	}
}
func TestRootAlreadyPrivileged(t *testing.T) {
	b, _ := json.Marshal(State{User: "root", Files: []File{{"/x", 0777, "root", "root"}}, Cron: []Job{{"root", "/x"}}})
	got, err := EscalationPaths(string(b))
	if err != nil || len(got) != 0 {
		t.Fatal(got, err)
	}
}

func TestPermissionClassSelection(t *testing.T) {
	for i, tc := range []struct {
		owner, group string
		mode         uint32
		yes          bool
	}{
		{"analyst", "ops", 0402, false}, {"analyst", "ops", 0600, true}, {"root", "ops", 0702, false}, {"root", "ops", 0720, true}, {"root", "other", 0702, true}, {"root", "other", 0770, false},
	} {
		s := State{User: "analyst", Groups: []string{"ops"}, Files: []File{{"/work", tc.mode, tc.owner, tc.group}}, Cron: []Job{{"root", "/work"}}}
		raw, _ := json.Marshal(s)
		got, err := EscalationPaths(string(raw))
		if err != nil || (len(got) == 1) != tc.yes {
			t.Fatal(i, got, err)
		}
	}
}
