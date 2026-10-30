package main

import (
	"fmt"
	"sort"
)

type Host struct{ Files, Keys, Tasks map[string]string }
type Step struct{ Kind, Name, Value string }
type Entry struct {
	Kind, Name string
	Existed    bool
	Old        string
}
type Journal struct{ Entries []Entry }

func table(h *Host, kind string) map[string]string {
	switch kind {
	case "file":
		return h.Files
	case "key":
		return h.Keys
	case "task":
		return h.Tasks
	}
	return nil
}

func validKind(kind string) bool { return kind == "file" || kind == "key" || kind == "task" }

func Apply(h *Host, j *Journal, steps []Step, crashAt int) (bool, error) {
	if h == nil || j == nil || len(j.Entries) != 0 {
		return false, fmt.Errorf("host missing or journal not empty")
	}
	if len(steps) == 0 || len(steps) > 32 {
		return false, fmt.Errorf("invalid step count")
	}
	for _, s := range steps {
		if !validKind(s.Kind) || len(s.Name) == 0 || len(s.Name) > 64 || len(s.Value) > 256 {
			return false, fmt.Errorf("invalid step")
		}
	}
	if h.Files == nil {
		h.Files = map[string]string{}
	}
	if h.Keys == nil {
		h.Keys = map[string]string{}
	}
	if h.Tasks == nil {
		h.Tasks = map[string]string{}
	}
	for i, s := range steps {
		if crashAt == 2*i {
			return true, nil
		}
		old, existed := table(h, s.Kind)[s.Name]
		j.Entries = append(j.Entries, Entry{s.Kind, s.Name, existed, old})
		if crashAt == 2*i+1 {
			return true, nil
		}
		table(h, s.Kind)[s.Name] = s.Value
	}
	return crashAt == 2*len(steps), nil
}

func Recover(h *Host, j *Journal) error {
	if h == nil || j == nil {
		return fmt.Errorf("host or journal missing")
	}
	for _, e := range j.Entries {
		if !validKind(e.Kind) {
			return fmt.Errorf("corrupt journal")
		}
	}
	for i := len(j.Entries) - 1; i >= 0; i-- {
		e := j.Entries[i]
		if h.Files == nil {
			h.Files = map[string]string{}
		}
		if h.Keys == nil {
			h.Keys = map[string]string{}
		}
		if h.Tasks == nil {
			h.Tasks = map[string]string{}
		}
		m := table(h, e.Kind)
		if e.Existed {
			m[e.Name] = e.Old
		} else {
			delete(m, e.Name)
		}
	}
	j.Entries = nil
	return nil
}

func Audit(h, baseline *Host) []string {
	out := []string{}
	for _, kind := range []string{"file", "key", "task"} {
		var a, b map[string]string
		if h != nil {
			a = table(h, kind)
		}
		if baseline != nil {
			b = table(baseline, kind)
		}
		dir := kind + "s/"
		for name, v := range a {
			if bv, ok := b[name]; !ok {
				out = append(out, dir+name+" added")
			} else if bv != v {
				out = append(out, dir+name+" changed")
			}
		}
		for name := range b {
			if _, ok := a[name]; !ok {
				out = append(out, dir+name+" removed")
			}
		}
	}
	sort.Strings(out)
	return out
}

func main() {
	h := &Host{Files: map[string]string{"notes.txt": "original"}}
	base := &Host{Files: map[string]string{"notes.txt": "original"}}
	j := &Journal{}
	crashed, err := Apply(h, j, []Step{{"file", "notes.txt", "changed"}, {"task", "tidy", "daily"}}, 3)
	fmt.Println(crashed, err, Audit(h, base))
	fmt.Println(Recover(h, j), Audit(h, base))
}
