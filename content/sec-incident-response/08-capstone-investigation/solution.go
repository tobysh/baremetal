package main

import (
	"sort"
	"strconv"
	"strings"
	"time"
)

type File struct {
	Path, Hash, Owner string
	Mode              uint32
	MTime, CTime      int64
}

type Edge struct{ From, To string }

type Audit struct {
	ID              string
	Time            int64
	Actor, From, To string
}

type Case struct {
	Auth, Web     []string
	Base, Now     []File
	Allowed       map[string]string
	Before, After []Edge
	Audit         []Audit
}

type Finding struct {
	Kind, Subject string
	Cites         []string
}

type Report struct {
	Findings    []Finding
	Timeline    []string
	Assumptions []string
}

type rec struct {
	id, user, ip, path string
	t, bytes           int64
	ok                 bool
}

func kv(fields []string) map[string]string {
	m := map[string]string{}
	for _, f := range fields {
		if k, v, ok := strings.Cut(f, "="); ok {
			m[k] = v
		}
	}
	return m
}

func parse(lines []string, web bool) []rec {
	var out []rec
	for _, l := range lines {
		f := strings.Fields(l)
		if len(f) < 4 {
			continue
		}
		t, err := time.Parse(time.RFC3339, f[1])
		if err != nil {
			continue
		}
		m := kv(f[2:])
		r := rec{id: f[0], t: t.Unix(), user: m["user"], ip: m["ip"]}
		if web {
			r.bytes, _ = strconv.ParseInt(m["bytes"], 10, 64)
			r.path = f[len(f)-1]
		} else {
			r.ok = f[len(f)-1] == "ok"
		}
		out = append(out, r)
	}
	return out
}

func reach(edges []Edge, target string) map[string]bool {
	rev := map[string][]string{}
	for _, e := range edges {
		rev[e.To] = append(rev[e.To], e.From)
	}
	seen := map[string]bool{target: true}
	q := []string{target}
	for len(q) > 0 {
		n := q[0]
		q = q[1:]
		for _, f := range rev[n] {
			if !seen[f] {
				seen[f] = true
				q = append(q, f)
			}
		}
	}
	return seen
}

func Investigate(c Case) Report {
	auth, web := parse(c.Auth, false), parse(c.Web, true)
	times := map[string]int64{}
	for _, r := range auth {
		times[r.id] = r.t
	}
	for _, r := range web {
		times[r.id] = r.t
	}
	var fs []Finding
	var first *rec
	for i := range auth {
		s := auth[i]
		if !s.ok {
			continue
		}
		cites := []string{s.id}
		n := 0
		for _, f := range auth {
			if !f.ok && f.ip == s.ip && f.t >= s.t-600 && f.t < s.t {
				n++
				cites = append(cites, f.id)
			}
		}
		if n >= 5 {
			fs = append(fs, Finding{"initial-access", s.user, cites})
			if first == nil || s.t < first.t {
				first = &auth[i]
			}
		}
	}
	if first == nil {
		return Report{}
	}
	T, victim, ip := first.t, first.user, first.ip
	base := map[string]File{}
	for _, f := range c.Base {
		base[f.Path] = f
	}
	for _, f := range c.Now {
		b, had := base[f.Path]
		if had && b.Hash == f.Hash {
			continue
		}
		if c.Allowed[f.Path] == f.Hash {
			continue
		}
		auto := strings.HasPrefix(f.Path, "/etc/cron.d/") || strings.HasPrefix(f.Path, "/etc/systemd/system/") || strings.HasSuffix(f.Path, "/.ssh/authorized_keys")
		if auto && f.CTime >= T {
			id := "F:" + f.Path
			times[id] = f.CTime
			fs = append(fs, Finding{"persistence", f.Path, []string{id}})
		}
	}
	rb, ra := reach(c.Before, "admins"), reach(c.After, "admins")
	had := map[Edge]bool{}
	for _, e := range c.Before {
		had[e] = true
	}
	in := map[Edge]bool{}
	for _, e := range c.After {
		in[e] = true
	}
	for _, a := range c.Audit {
		e := Edge{a.From, a.To}
		if a.Actor == victim && a.Time >= T && in[e] && !had[e] && ra[a.To] && !rb[a.From] {
			times[a.ID] = a.Time
			fs = append(fs, Finding{"privilege-change", a.From + ">" + a.To, []string{a.ID}})
		}
	}
	var sum int64
	var ex []string
	for _, w := range web {
		if w.ip == ip && w.t >= T && strings.HasPrefix(w.path, "/export/") {
			sum += w.bytes
			ex = append(ex, w.id)
		}
	}
	if sum >= 1000000 {
		fs = append(fs, Finding{"exfiltration", ip, ex})
	}
	sort.Slice(fs, func(i, j int) bool {
		if fs[i].Kind != fs[j].Kind {
			return fs[i].Kind < fs[j].Kind
		}
		return fs[i].Subject < fs[j].Subject
	})
	seen := map[string]bool{}
	var tl []string
	for _, f := range fs {
		for _, id := range f.Cites {
			if !seen[id] {
				seen[id] = true
				tl = append(tl, id)
			}
		}
	}
	sort.Slice(tl, func(i, j int) bool {
		if times[tl[i]] != times[tl[j]] {
			return times[tl[i]] < times[tl[j]]
		}
		return tl[i] < tl[j]
	})
	return Report{fs, tl, []string{"Clocks are synchronized and the supplied records are complete and unaltered."}}
}

func main() {}
