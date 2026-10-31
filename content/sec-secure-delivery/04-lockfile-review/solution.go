package main

import (
	"fmt"
	"sort"
	"strconv"
	"strings"
)

type Pkg struct {
	Name    string
	Version [3]int
	Digest  string // "sha256:<hex>"
	Source  string
}

type Finding struct {
	Pkg, Kind, Severity, Detail string
}

type Policy struct {
	AllowedSources map[string]bool
	Popular        []string // well-known names to guard against look-alikes
}

func cmpVersion(a, b [3]int) int {
	for i := 0; i < 3; i++ {
		if a[i] != b[i] {
			if a[i] < b[i] {
				return -1
			}
			return 1
		}
	}
	return 0
}

func parseVersion(s string) ([3]int, bool) {
	var v [3]int
	parts := strings.Split(s, ".")
	if len(parts) != 3 {
		return v, false
	}
	for i, p := range parts {
		n, err := strconv.Atoi(p)
		if err != nil || n < 0 || strconv.Itoa(n) != p {
			return v, false
		}
		v[i] = n
	}
	return v, true
}

// ParseLock is strict: every problem is an error naming the line.
func ParseLock(text string) (map[string]Pkg, error) {
	out := map[string]Pkg{}
	for i, line := range strings.Split(text, "\n") {
		if line == "" || strings.HasPrefix(line, "#") {
			continue
		}
		f := strings.Split(line, " ")
		if len(f) != 5 || f[0] != "pkg" || f[1] == "" {
			return nil, fmt.Errorf("line %d: malformed", i+1)
		}
		v, ok := parseVersion(f[2])
		if !ok {
			return nil, fmt.Errorf("line %d: bad version %q", i+1, f[2])
		}
		d, ok := strings.CutPrefix(f[3], "sha256:")
		if !ok || len(d) != 64 || strings.Trim(d, "0123456789abcdef") != "" {
			return nil, fmt.Errorf("line %d: bad digest", i+1)
		}
		if _, dup := out[f[1]]; dup {
			return nil, fmt.Errorf("line %d: duplicate package %s", i+1, f[1])
		}
		out[f[1]] = Pkg{f[1], v, f[3], f[4]}
	}
	return out, nil
}

func oneEdit(a, b string) bool {
	if len(a) > len(b) {
		a, b = b, a
	}
	if len(b)-len(a) > 1 || a == b {
		return false
	}
	i := 0
	for i < len(a) && a[i] == b[i] {
		i++
	}
	if len(a) == len(b) {
		return a[i+1:] == b[i+1:]
	}
	return a[i:] == b[i+1:]
}

func confusable(n, m string) bool {
	if n == m {
		return false
	}
	norm := func(s string) string { return strings.ReplaceAll(strings.ToLower(s), "_", "-") }
	if norm(n) == norm(m) {
		return true
	}
	return len(n) >= 4 && len(m) >= 4 && oneEdit(n, m)
}

var kindOrder = []string{"added", "removed", "upgrade", "downgrade", "hash-changed", "source-changed", "source-not-allowed", "lookalike"}

func Review(oldText, newText string, policy Policy) ([]Finding, error) {
	oldP, err := ParseLock(oldText)
	if err != nil {
		return nil, fmt.Errorf("old lockfile: %w", err)
	}
	newP, err := ParseLock(newText)
	if err != nil {
		return nil, fmt.Errorf("new lockfile: %w", err)
	}
	names := map[string]bool{}
	for n := range oldP {
		names[n] = true
	}
	for n := range newP {
		names[n] = true
	}
	sorted := make([]string, 0, len(names))
	for n := range names {
		sorted = append(sorted, n)
	}
	sort.Strings(sorted)
	var out []Finding
	for _, n := range sorted {
		o, hadOld := oldP[n]
		c, hasNew := newP[n]
		var fs []Finding
		add := func(kind, sev, detail string) { fs = append(fs, Finding{n, kind, sev, detail}) }
		switch {
		case !hasNew:
			add("removed", "info", "was "+versionString(o.Version))
		case !hadOld:
			add("added", "info", versionString(c.Version)+" from "+c.Source)
		default:
			switch cmp := cmpVersion(o.Version, c.Version); {
			case cmp < 0:
				add("upgrade", "info", versionString(o.Version)+" -> "+versionString(c.Version))
			case cmp > 0:
				add("downgrade", "high", versionString(o.Version)+" -> "+versionString(c.Version))
			case o.Digest != c.Digest:
				add("hash-changed", "critical", "digest changed at "+versionString(c.Version))
			}
			if o.Source != c.Source {
				add("source-changed", "high", o.Source+" -> "+c.Source)
			}
		}
		if hasNew && !policy.AllowedSources[c.Source] {
			add("source-not-allowed", "high", c.Source)
		}
		if hasNew && !hadOld {
			var cands []string
			for m := range oldP {
				cands = append(cands, m)
			}
			cands = append(cands, policy.Popular...)
			sort.Strings(cands)
			for _, m := range cands {
				if confusable(n, m) {
					add("lookalike", "high", "resembles "+m)
					break
				}
			}
		}
		sort.SliceStable(fs, func(i, j int) bool { return kindIndex(fs[i].Kind) < kindIndex(fs[j].Kind) })
		out = append(out, fs...)
	}
	return out, nil
}

func kindIndex(k string) int {
	for i, v := range kindOrder {
		if v == k {
			return i
		}
	}
	return len(kindOrder)
}

func versionString(v [3]int) string { return fmt.Sprintf("%d.%d.%d", v[0], v[1], v[2]) }

const oldLock = `# bm-lock v1
pkg requests 2.3.1 sha256:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa registry.example.test
pkg fmtlib 1.0.0 sha256:bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb registry.example.test
`

const newLock = `# bm-lock v1
pkg requests 2.2.0 sha256:cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc registry.example.test
pkg fmtlib 1.0.0 sha256:dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd registry.example.test
pkg reqeusts 0.1.0 sha256:eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee mirror.example.test
`

func main() {
	p := Policy{AllowedSources: map[string]bool{"registry.example.test": true}}
	fs, err := Review(oldLock, newLock, p)
	fmt.Println("error:", err)
	for _, f := range fs {
		fmt.Printf("%-8s %-18s %-9s %s\n", f.Pkg, f.Kind, f.Severity, f.Detail)
	}
}
