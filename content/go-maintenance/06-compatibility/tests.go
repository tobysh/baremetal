package main

import (
	"reflect"
	"strings"
	"testing"
)

// Compile-time guard: these signatures are the public API old callers use.
var (
	_ func(string) (*Config, error)  = Parse
	_ func(*Config, string) string   = (*Config).Get
	_ func(*Config, string) bool     = (*Config).Has
	_ func(*Config) []string         = (*Config).Keys
	_ func(*Config, string, int) int = (*Config).Int
)

// Legacy fixtures: inputs and the results the released parser produced.
var legacy = []struct {
	name string
	src  string
	keys []string
	vals map[string]string
	err  string
}{
	{"basic", "a=1\nb = two\n", []string{"a", "b"}, map[string]string{"a": "1", "b": "two"}, ""},
	{"comments", "# x\n; y\n\n a = 1 \n", []string{"a"}, map[string]string{"a": "1"}, ""},
	{"inline comment", "a = x # note\nb = y#z\nc = # only\nd = e #\n", []string{"a", "b", "c", "d"}, map[string]string{"a": "x", "b": "y#z", "c": "", "d": "e"}, ""},
	{"quotes are literal", `a = "hi"` + "\n" + `b = "x # y"` + "\n", []string{"a", "b"}, map[string]string{"a": `"hi"`, "b": `"x`}, ""},
	{"duplicate keys", "a=1\nb=2\na=3\n", []string{"a", "b"}, map[string]string{"a": "3", "b": "2"}, ""},
	{"equals in value", "url = http://h/?q=1\n", []string{"url"}, map[string]string{"url": "http://h/?q=1"}, ""},
	{"crlf", "a=1\r\nb=2\r\n", []string{"a", "b"}, map[string]string{"a": "1", "b": "2"}, ""},
	{"missing equals", "a=1\nnonsense\n", nil, nil, "line 2: missing '='"},
	{"empty key", "\n = v\n", nil, nil, "line 2: empty key"},
	{"empty input", "", nil, map[string]string{}, ""},
	{"case sensitive", "A=1\na=2\n", []string{"A", "a"}, map[string]string{"A": "1", "a": "2"}, ""},
}

func TestParse_legacy_fixtures(t *testing.T) {
	for _, f := range legacy {
		for _, parse := range []struct {
			label string
			fn    func() (*Config, error)
		}{
			{"Parse", func() (*Config, error) { return Parse(f.src) }},
			{"ParseOpts{}", func() (*Config, error) { return ParseOpts(f.src, Options{}) }},
		} {
			c, err := parse.fn()
			if f.err != "" {
				if err == nil || err.Error() != f.err || c != nil {
					t.Errorf("%s %s: got %v, %v; want error %q", parse.label, f.name, c, err, f.err)
				}
				continue
			}
			if err != nil {
				t.Errorf("%s %s: unexpected error %v", parse.label, f.name, err)
				continue
			}
			if !reflect.DeepEqual(c.Keys(), f.keys) {
				t.Errorf("%s %s: Keys = %v; want %v", parse.label, f.name, c.Keys(), f.keys)
			}
			for k, v := range f.vals {
				if c.Get(k) != v || !c.Has(k) {
					t.Errorf("%s %s: Get(%q) = %q; want %q", parse.label, f.name, k, c.Get(k), v)
				}
			}
		}
	}
}

func TestParse_never_reads_quotes(t *testing.T) {
	c, err := Parse(`a = "x # y"` + "\n")
	if err != nil || c.Get("a") != `"x` {
		t.Errorf("Parse changed meaning: %q, %v", c.Get("a"), err)
	}
}

func TestInt_and_missing_legacy(t *testing.T) {
	c, _ := Parse("n = 42\nbad = 4x\nhex = 0x10\nneg = -7\nempty =\n")
	for _, tc := range []struct {
		k    string
		def  int
		want int
	}{{"n", 0, 42}, {"bad", 9, 9}, {"hex", 5, 5}, {"neg", 0, -7}, {"empty", 3, 3}, {"missing", 8, 8}} {
		if got := c.Int(tc.k, tc.def); got != tc.want {
			t.Errorf("Int(%q, %d) = %d; want %d", tc.k, tc.def, got, tc.want)
		}
	}
	if c.Get("missing") != "" || c.Has("missing") {
		t.Errorf("missing key reported present")
	}
}

func TestQuotes_new_behaviour(t *testing.T) {
	src := "a = \"x # y\"\n" +
		"b = \"  padded  \" # comment\n" +
		"c = \"say \\\"hi\\\"\\\\\"\n" +
		"d = \"line1\\nline2\"\n" +
		"e = plain # c\n" +
		"f = \"\"\n" +
		"g = z\"q\"\n"
	c, err := ParseOpts(src, Options{Quotes: true})
	if err != nil {
		t.Fatal(err)
	}
	want := map[string]string{
		"a": "x # y", "b": "  padded  ", "c": `say "hi"\`, "d": "line1\nline2", "e": "plain", "f": "", "g": `z"q"`,
	}
	for k, v := range want {
		if c.Get(k) != v {
			t.Errorf("Get(%q) = %q; want %q", k, c.Get(k), v)
		}
	}
	if !reflect.DeepEqual(c.Keys(), []string{"a", "b", "c", "d", "e", "f", "g"}) {
		t.Errorf("Keys = %v", c.Keys())
	}
}

func TestQuotes_errors(t *testing.T) {
	for _, tc := range []struct{ src, want string }{
		{"a = 1\nb = \"open\n", "line 2: unterminated quote"},
		{"b = \"x\\\n", "line 1: unterminated quote"},
		{"b = \"x\\q\"\n", "line 1: bad escape"},
		{"\nb = \"x\" y\n", "line 2: junk after quoted value"},
		{"b = \"x\"#c\n", "line 1: junk after quoted value"},
		{"b = \"x\"\nc\n", "line 2: missing '='"},
	} {
		c, err := ParseOpts(tc.src, Options{Quotes: true})
		if err == nil || err.Error() != tc.want || c != nil {
			t.Errorf("ParseOpts(%q) = %v, %v; want error %q", tc.src, c, err, tc.want)
		}
		// Without the option the same input must keep its legacy meaning.
		if strings.Contains(tc.want, "missing") {
			continue
		}
		if _, err := Parse(tc.src); err != nil {
			t.Errorf("legacy Parse(%q) started failing: %v", tc.src, err)
		}
	}
}

func TestBool(t *testing.T) {
	c, _ := Parse("a=TRUE\nb=No\nc=on\nd=0\ne=maybe\nf=\ng= yes \nh=OFF\ni=1\n")
	for _, tc := range []struct {
		k         string
		def, want bool
	}{
		{"a", false, true}, {"b", true, false}, {"c", false, true}, {"d", true, false},
		{"e", true, true}, {"e", false, false}, {"f", true, true}, {"g", false, true},
		{"h", true, false}, {"i", false, true}, {"missing", true, true}, {"missing", false, false},
	} {
		if got := c.Bool(tc.k, tc.def); got != tc.want {
			t.Errorf("Bool(%q, %v) = %v; want %v", tc.k, tc.def, got, tc.want)
		}
	}
}
