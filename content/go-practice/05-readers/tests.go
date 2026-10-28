package main

import (
	"errors"
	"reflect"
	"strings"
	"testing"
	"testing/iotest"
)

func TestParseKV_basic(t *testing.T) {
	m, err := ParseKV(strings.NewReader("name = Ada\nlang=Go\n  year  =  1815  \n"))
	want := map[string]string{"name": "Ada", "lang": "Go", "year": "1815"}
	if err != nil || !reflect.DeepEqual(m, want) {
		t.Errorf("ParseKV = %v, %v; want %v, nil", m, err, want)
	}
}

func TestParseKV_comments_blanks_and_duplicates(t *testing.T) {
	in := "# settings\n\n   # indented comment\na=1\n\na=2\nb=\n"
	m, err := ParseKV(strings.NewReader(in))
	want := map[string]string{"a": "2", "b": ""}
	if err != nil || !reflect.DeepEqual(m, want) {
		t.Errorf("ParseKV = %v, %v; want %v, nil (later duplicates win, empty values are fine)", m, err, want)
	}
}

func TestParseKV_value_may_contain_equals(t *testing.T) {
	m, err := ParseKV(strings.NewReader("url = http://x.test/?a=1&b=2"))
	if err != nil || m["url"] != "http://x.test/?a=1&b=2" {
		t.Errorf("ParseKV = %v, %v; only the first '=' separates key from value", m, err)
	}
}

func TestParseKV_empty_input(t *testing.T) {
	m, err := ParseKV(strings.NewReader(""))
	if err != nil || m == nil || len(m) != 0 {
		t.Errorf("ParseKV(\"\") = %#v, %v; want an empty, non-nil map and nil", m, err)
	}
}

func TestParseKV_errors_have_line_numbers(t *testing.T) {
	m, err := ParseKV(strings.NewReader("a=1\n\n# note\nbroken line\nb=2\n"))
	if m != nil || err == nil || err.Error() != "line 4: missing '='" {
		t.Errorf("ParseKV = %v, %v; want nil and \"line 4: missing '='\" (blank and comment lines count)", m, err)
	}
	m, err = ParseKV(strings.NewReader("a=1\n  = oops\n"))
	if m != nil || err == nil || err.Error() != "line 2: empty key" {
		t.Errorf("ParseKV = %v, %v; want nil and \"line 2: empty key\"", m, err)
	}
}

func TestParseKV_reports_read_errors(t *testing.T) {
	boom := errors.New("disk fell off")
	r := iotest.ErrReader(boom)
	m, err := ParseKV(r)
	if m != nil || !errors.Is(err, boom) {
		t.Errorf("ParseKV on a failing reader = %v, %v; want nil and the read error (check sc.Err())", m, err)
	}
}

func TestStats_counts(t *testing.T) {
	cases := []struct {
		in                string
		lines, words, byt int
	}{
		{"", 0, 0, 0},
		{"hello", 1, 1, 5},
		{"one two\nthree\n", 2, 3, 14},
		{"a\n\nb c d", 3, 4, 8},
		{"  spaced   out \n", 1, 2, 16},
	}
	for _, c := range cases {
		l, w, b, err := Stats(strings.NewReader(c.in))
		if err != nil || l != c.lines || w != c.words || b != c.byt {
			t.Errorf("Stats(%q) = %d, %d, %d, %v; want %d, %d, %d, nil", c.in, l, w, b, err, c.lines, c.words, c.byt)
		}
	}
}

func TestStats_reports_read_errors(t *testing.T) {
	boom := errors.New("nope")
	_, _, _, err := Stats(iotest.ErrReader(boom))
	if !errors.Is(err, boom) {
		t.Errorf("Stats on a failing reader returned err = %v; want the read error", err)
	}
}
