package main

import (
	"crypto/sha256"
	"fmt"
	"regexp"
	"sort"
	"strings"
)

type Finding struct {
	Line, Col   int // 1-based
	Rule        string
	Preview     string
	Fingerprint string
}

// Preview shows at most the first 4 bytes of a secret. It never shows more.
func Preview(secret string) string {
	n := len(secret)
	if n > 4 {
		n = 4
	}
	return secret[:n] + "****"
}

// Fingerprint is the first 12 hex digits of SHA-256 of the secret.
func Fingerprint(secret string) string {
	return fmt.Sprintf("%x", sha256.Sum256([]byte(secret)))[:12]
}

var (
	tokenRE  = regexp.MustCompile(`\bstk_(?:live|test)_[A-Za-z0-9]{24}\b`)
	assignRE = regexp.MustCompile(`(?i)(password|passwd|secret|api[_-]?key|token)[A-Za-z0-9_-]*\s*[:=]\s*["']?([^\s"',;]+)`)
)

const header = "-----BEGIN SYNTHETIC PRIVATE KEY-----"

func placeholder(v string) bool {
	for _, p := range []string{"${", "$(", "<", "{{", "%", "env:"} {
		if strings.HasPrefix(v, p) {
			return true
		}
	}
	l := strings.ToLower(v)
	for _, p := range []string{"example", "placeholder", "redacted", "changeme"} {
		if strings.Contains(l, p) {
			return true
		}
	}
	return false
}

func distinct(v string) int {
	var seen [256]bool
	n := 0
	for i := 0; i < len(v); i++ {
		if !seen[v[i]] {
			seen[v[i]] = true
			n++
		}
	}
	return n
}

func Scan(text string, baseline map[string]bool) []Finding {
	var out []Finding
	for i, line := range strings.Split(text, "\n") {
		line = strings.TrimSuffix(line, "\r")
		var cand []Finding
		var secrets []string
		add := func(col int, rule, secret string) {
			if baseline[Fingerprint(secret)] {
				return
			}
			cand = append(cand, Finding{i + 1, col + 1, rule, Preview(secret), Fingerprint(secret)})
			secrets = append(secrets, secret)
		}
		if at := strings.Index(line, header); at >= 0 {
			add(at, "private-key", header)
		}
		tokStarts := map[int]bool{}
		for _, m := range tokenRE.FindAllStringIndex(line, -1) {
			tokStarts[m[0]] = true
			add(m[0], "svc-token", line[m[0]:m[1]])
		}
		for _, m := range assignRE.FindAllStringSubmatchIndex(line, -1) {
			v := line[m[4]:m[5]]
			if tokStarts[m[4]] || len(v) < 8 || distinct(v) < 6 || placeholder(v) {
				continue
			}
			add(m[4], "assignment", v)
		}
		sort.SliceStable(cand, func(a, b int) bool { return cand[a].Col < cand[b].Col })
		out = append(out, cand...)
	}
	return out
}

// Try it with Run. Every value below is invented.
func main() {
	cfg := "name = demo\n" +
		"api_key = 'abcd1234wxyz'\n" +
		"password: ${DB_PASSWORD}\n" +
		"url = https://x.example.test/?t=stk_live_Zk3f9QaB7cD2eF5gH8jK1mN4\n"
	for _, f := range Scan(cfg, nil) {
		fmt.Printf("%d:%d %-10s %s %s\n", f.Line, f.Col, f.Rule, f.Preview, f.Fingerprint)
	}
}
