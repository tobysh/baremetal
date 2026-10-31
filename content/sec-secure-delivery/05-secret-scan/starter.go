package main

import (
	"crypto/sha256"
	"fmt"
	"regexp"
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

var _ = strings.Contains // you will want strings

func Scan(text string, baseline map[string]bool) []Finding {
	return nil
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
