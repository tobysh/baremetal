package main

import (
	"strings"
	"testing"
)

// norm is the original behaviour with optional single-line defects. m == 0 is
// the original; 1..14 are the mutants the learner's inputs must expose.
func norm(s string, m int) string {
	if m != 13 {
		s = strings.TrimSpace(s)
	}
	ext := ""
	low := strings.ToLower(s)
	if m == 11 {
		low = s
	}
	markers := []string{"ext.", "ext", "x"}
	if m == 10 {
		markers = []string{"ext", "x"}
	}
	for _, mk := range markers {
		if i := strings.LastIndex(low, mk); i > 0 {
			d := s[i+len(mk):]
			if m != 14 {
				d = strings.TrimSpace(d)
			}
			lim := 5
			if m == 1 {
				lim = 6
			}
			if d == "" || len(d) > lim || (m != 12 && !digits(d)) {
				return ""
			}
			ext = ";ext=" + d
			s = s[:i]
			break
		}
	}
	var b strings.Builder
	for _, r := range s {
		if r == ' ' || r == '-' || r == '(' || r == ')' || (r == '.' && m != 13) {
			continue
		}
		b.WriteRune(r)
	}
	n := b.String()
	if m != 6 && strings.HasPrefix(n, "00") {
		n = "+" + n[2:]
	}
	if strings.HasPrefix(n, "+") {
		if !digits(n[1:]) {
			return ""
		}
	} else {
		if !digits(n) {
			return ""
		}
		switch {
		case len(n) == 11 && n[0] == '0':
			if m == 7 {
				n = "+44" + n
			} else {
				n = "+44" + n[1:]
			}
		case len(n) == 10 && (n[0] != '0' || m == 8):
			n = "+1" + n
		case len(n) == 11 && (n[0] == '1' || m == 9):
			n = "+" + n
		default:
			return ""
		}
	}
	d := len(n) - 1
	minD, maxD := 8, 15
	switch m {
	case 2:
		minD = 7
	case 3:
		minD = 9
	case 4:
		maxD = 16
	case 5:
		maxD = 14
	}
	if d < minD || d > maxD {
		return ""
	}
	return n + ext
}

func digits(s string) bool {
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

const mutants = 14

func TestOriginal_still_matches_reference(t *testing.T) {
	for _, c := range []struct{ in, want string }{
		{"(415) 555-0132", "+14155550132"},
		{"020 7946 0958 x12", "+442079460958;ext=12"},
		{"0049 30 1234567", "+49301234567"},
		{"12", ""},
		{"14155550132", "+14155550132"},
		{"+1234567", ""},
		{"4155550132 ext. 7", "+14155550132;ext=7"},
	} {
		if got := norm(c.in, 0); got != c.want {
			t.Fatalf("test harness bug: reference gives %q for %q; want %q", got, c.in, c.want)
		}
		if got := Normalize(c.in); got != c.want {
			t.Errorf("Normalize(%q) = %q; want %q. This lesson is read-only: restore the original", c.in, got, c.want)
		}
	}
}

func TestCases_budget_and_integrity(t *testing.T) {
	cs := Cases()
	if len(cs) == 0 {
		t.Fatalf("Cases() is empty")
	}
	if len(cs) > 30 {
		t.Errorf("Cases() has %d inputs; the limit is 30", len(cs))
	}
	for _, in := range cs {
		if got, want := Normalize(in), norm(in, 0); got != want {
			t.Errorf("Normalize(%q) = %q, the original gives %q: you changed the function", in, got, want)
		}
	}
}

func TestCases_expose_every_mutant(t *testing.T) {
	cs := Cases()
	missed := 0
	for m := 1; m <= mutants; m++ {
		killed := false
		for _, in := range cs {
			if norm(in, m) != norm(in, 0) {
				killed = true
				break
			}
		}
		if !killed {
			missed++
			t.Errorf("hidden broken copy #%d of %d gives the same answers as the original on all your inputs. Which branch or boundary have you not pinned?", m, mutants)
		}
	}
	if missed == 0 && len(cs) > 0 {
		t.Logf("all %d mutants exposed with %d inputs", mutants, len(cs))
	}
}
