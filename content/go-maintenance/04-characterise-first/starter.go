package main

import (
	"fmt"
	"strings"
)

// Normalize turns a phone number as typed into a canonical form:
// "+<digits>" with an optional ";ext=<digits>". It returns "" for input
// it cannot make sense of. Legacy code: read it, do not change it.
func Normalize(s string) string {
	s = strings.TrimSpace(s)
	ext := ""
	low := strings.ToLower(s)
	for _, marker := range []string{"ext.", "ext", "x"} {
		if i := strings.LastIndex(low, marker); i > 0 {
			digits := s[i+len(marker):]
			digits = strings.TrimSpace(digits)
			if digits == "" || len(digits) > 5 || !allDigits(digits) {
				return ""
			}
			ext = ";ext=" + digits
			s = s[:i]
			break
		}
	}
	var b strings.Builder
	for _, r := range s {
		switch r {
		case ' ', '-', '.', '(', ')':
			continue
		}
		b.WriteRune(r)
	}
	n := b.String()
	if strings.HasPrefix(n, "00") {
		n = "+" + n[2:]
	}
	if strings.HasPrefix(n, "+") {
		if !allDigits(n[1:]) {
			return ""
		}
	} else {
		if !allDigits(n) {
			return ""
		}
		switch {
		case len(n) == 11 && n[0] == '0':
			n = "+44" + n[1:]
		case len(n) == 10 && n[0] != '0':
			n = "+1" + n
		case len(n) == 11 && n[0] == '1':
			n = "+" + n
		default:
			return ""
		}
	}
	if d := len(n) - 1; d < 8 || d > 15 {
		return ""
	}
	return n + ext
}

func allDigits(s string) bool {
	if s == "" {
		return false
	}
	for _, r := range s {
		if r < '0' || r > '9' {
			return false
		}
	}
	return true
}

// Cases returns the inputs that characterise Normalize.
func Cases() []string {
	return nil
}

func main() {
	for _, in := range []string{"(415) 555-0132", "020 7946 0958 x12", "0049 30 1234567", "12"} {
		fmt.Printf("%q -> %q\n", in, Normalize(in))
	}
}
