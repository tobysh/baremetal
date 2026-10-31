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
	return []string{
		"+441234567890",     // plus form, valid
		"+1234567",          // 7 digits: too short
		"+12345678",         // 8 digits: shortest allowed
		"+123456789012345",  // 15 digits: longest allowed
		"+1234567890123456", // 16 digits: too long
		"+44 12a4567890",    // letter after plus
		"0044 20 7946 0958", // 00 prefix
		"02079460958",       // national UK, 11 digits
		"4155550132",        // US 10 digits
		"04155550132 ",      // surrounding space, UK style
		"14155550132",       // 11 digits starting 1
		"24155550132",       // 11 digits other
		"0415555013",        // 10 digits starting 0
		"(415) 555-0132",    // separators
		"415.555.0132",      // dots
		"+44 (0) 20-7946.0958",
		"4155550132x12345",  // extension 5 digits
		"4155550132x123456", // extension 6 digits
		"4155550132 X 12",   // upper case X
		"4155550132 EXT 7",  // upper case EXT
		"4155550132 ext. 7", // ext. spelling
		"4155550132 ext.",   // ext with no digits
		"4155550132x1a",     // ext not digits
		"x12",               // ext marker at index 0
		"4155550132x1x2",    // last marker wins
		"",                  // empty
		"abc",               // letters
		"+1x5",              // short with ext
	}
}

func main() {
	for _, in := range []string{"(415) 555-0132", "020 7946 0958 x12", "0049 30 1234567", "12"} {
		fmt.Printf("%q -> %q\n", in, Normalize(in))
	}
}
