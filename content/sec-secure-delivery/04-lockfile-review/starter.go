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

var _ = sort.Strings // you will want sort

func Review(oldText, newText string, policy Policy) ([]Finding, error) {
	return nil, nil
}

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
