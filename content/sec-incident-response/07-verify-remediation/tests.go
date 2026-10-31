package main

import (
	"fmt"
	"testing"
)

func TestPublic(t *testing.T) {
	r, rec := Verify([]Finding{{"f1", "file", "/etc/cron.d/x", 0}}, Snapshot{Time: 200, Covers: map[string]bool{"file": true}}, 100, nil, nil)
	if r[0].Status != "closed" || rec {
		t.Fatal(r, rec)
	}
}

func TestMatrix(t *testing.T) {
	for i := 0; i < 12; i++ {
		fixed := int64(1000 + i*50)
		ref := func(s string) string { return fmt.Sprint(s, "-", i) }
		fs := []Finding{
			{"a", "account", ref("tmpuser"), 0}, {"b", "account", ref("gone"), 0}, {"c", "key", ref("fp"), 0},
			{"d", "file", ref("/etc/cron.d/x"), 0}, {"e", "credential", ref("svc"), 3},
			{"f", "credential", ref("svc2"), 3}, {"g", "credential", ref("svc3"), 3}, {"h", "credential", ref("nodata"), 3},
			{"i", "file", ref("/var/z"), 0},
		}
		snap := Snapshot{
			Time:        fixed + 10,
			Covers:      map[string]bool{"account": true, "key": true, "file": true, "credential": true},
			Accounts:    []string{ref("tmpuser"), "root"},
			Keys:        []string{"other"},
			Files:       []string{"/etc/passwd"},
			CredVersion: map[string]int{ref("svc"): 4, ref("svc2"): 4, ref("svc3"): 3},
			OldRejected: map[string]bool{ref("svc"): true, ref("svc2"): false, ref("svc3"): true},
		}
		want := "open closed closed closed closed open open unverifiable closed"
		check := func(label, want string, snap Snapshot, rec bool, act []Activity) {
			r, got := Verify(fs, snap, fixed, []string{"203.0.113.9"}, act)
			s := ""
			for k, x := range r {
				if x.ID != fs[k].ID {
					t.Fatal("order", r)
				}
				if k > 0 {
					s += " "
				}
				s += x.Status
			}
			if s != want || got != rec {
				t.Fatalf("%s case %d: got %q rec=%v want %q rec=%v", label, i, s, got, want, rec)
			}
		}
		check("base", want, snap, false, []Activity{{fixed - 5, "203.0.113.9"}, {fixed + 5, "198.51.100.1"}, {fixed, "203.0.113.9"}})
		check("recurrence", want, snap, true, []Activity{{fixed + 1, "203.0.113.9"}})
		stale := snap
		stale.Time = fixed - 1
		u := "unverifiable unverifiable unverifiable unverifiable unverifiable unverifiable unverifiable unverifiable unverifiable"
		check("stale", u, stale, false, nil)
		nc := snap
		nc.Covers = map[string]bool{"account": true}
		check("coverage", "open closed unverifiable unverifiable unverifiable unverifiable unverifiable unverifiable unverifiable", nc, false, nil)
		eq := snap
		eq.Time = fixed
		check("snapshot at fix time", want, eq, false, nil)
	}
}
