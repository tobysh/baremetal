package main

import (
	"encoding/json"
	"sort"
)

type File struct {
	Path         string
	Mode         uint32
	Owner, Group string
}
type Job struct{ RunAs, Path string }
type SudoRule struct {
	User, RunAs, Path string
	NoPassword        bool
}
type State struct {
	User   string
	Groups []string
	Files  []File
	Cron   []Job
	Sudo   []SudoRule
}

func EscalationPaths(raw string) ([]string, error) {
	var s State
	if err := json.Unmarshal([]byte(raw), &s); err != nil {
		return nil, err
	}
	out := []string{}
	if s.User == "root" {
		return out, nil
	}
	groups := map[string]bool{}
	for _, g := range s.Groups {
		groups[g] = true
	}
	writable := map[string]bool{}
	for _, f := range s.Files {
		bit := uint32(0002)
		if f.Owner == s.User {
			bit = 0200
		} else if groups[f.Group] {
			bit = 0020
		}
		writable[f.Path] = f.Mode&bit != 0
	}
	found := map[string]bool{}
	for _, j := range s.Cron {
		if j.RunAs == "root" && writable[j.Path] {
			found[j.Path] = true
		}
	}
	for _, r := range s.Sudo {
		if r.User == s.User && r.RunAs == "root" && r.NoPassword && writable[r.Path] {
			found[r.Path] = true
		}
	}
	for p := range found {
		out = append(out, p)
	}
	sort.Strings(out)
	return out, nil
}

func main() {}
