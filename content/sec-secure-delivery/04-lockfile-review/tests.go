package main

import (
	"fmt"
	"reflect"
	"strings"
	"testing"
)

func h(c string) string { return "sha256:" + strings.Repeat(c, 64) }

func lock(lines ...string) string {
	return "# bm-lock v1\n" + strings.Join(lines, "\n") + "\n"
}

func pk(name, ver, d, src string) string {
	return fmt.Sprintf("pkg %s %s %s %s", name, ver, h(d), src)
}

const R = "registry.example.test"

var pol = Policy{AllowedSources: map[string]bool{R: true}, Popular: []string{"serde-lite", "httpx"}}

type fk struct{ Pkg, Kind, Sev string }

func kinds(fs []Finding) []fk {
	out := []fk{}
	for _, f := range fs {
		out = append(out, fk{f.Pkg, f.Kind, f.Severity})
	}
	return out
}

func expect(t *testing.T, name, a, b string, p Policy, want []fk) {
	t.Helper()
	fs, err := Review(a, b, p)
	if err != nil {
		t.Errorf("%s: %v", name, err)
		return
	}
	if got := kinds(fs); !reflect.DeepEqual(got, want) && !(len(got) == 0 && len(want) == 0) {
		t.Errorf("%s:\n got  %v\n want %v", name, got, want)
	}
}

func TestNoChange(t *testing.T) {
	l := lock(pk("alpha", "1.0.0", "a", R), pk("beta", "2.1.0", "b", R))
	expect(t, "identical", l, l, pol, nil)
	expect(t, "reordered and commented", l, "# other comment\n\n"+pk("beta", "2.1.0", "b", R)+"\n"+pk("alpha", "1.0.0", "a", R)+"\n", pol, nil)
}

func TestVersionMovement(t *testing.T) {
	a := lock(pk("up", "1.9.9", "1", R), pk("down", "1.10.0", "2", R), pk("gone", "0.1.0", "3", R), pk("same", "1.0.0", "4", R))
	b := lock(pk("up", "1.10.0", "5", R), pk("down", "1.9.0", "6", R), pk("fresh", "0.0.1", "7", R), pk("same", "1.0.0", "4", R))
	expect(t, "moves", a, b, pol, []fk{
		{"down", "downgrade", "high"}, {"fresh", "added", "info"},
		{"gone", "removed", "info"}, {"up", "upgrade", "info"},
	})
}

func TestVersionComparisonIsNumeric(t *testing.T) {
	a := lock(pk("pkgx", "1.2.9", "1", R), pk("pkgy", "2.0.0", "2", R))
	b := lock(pk("pkgx", "1.2.10", "3", R), pk("pkgy", "10.0.0", "4", R))
	expect(t, "string order would call these downgrades", a, b, pol, []fk{{"pkgx", "upgrade", "info"}, {"pkgy", "upgrade", "info"}})
}

func TestHashChangeAtSameVersion(t *testing.T) {
	a := lock(pk("core", "3.0.1", "1", R), pk("util", "1.0.0", "2", R))
	b := lock(pk("core", "3.0.1", "9", R), pk("util", "1.0.0", "2", R))
	expect(t, "swap", a, b, pol, []fk{{"core", "hash-changed", "critical"}})
	// A different hash with a different version is just an upgrade.
	c := lock(pk("core", "3.0.2", "9", R), pk("util", "1.0.0", "2", R))
	expect(t, "normal upgrade", a, c, pol, []fk{{"core", "upgrade", "info"}})
}

func TestSources(t *testing.T) {
	a := lock(pk("core", "1.0.0", "1", R), pk("odd", "1.0.0", "2", "mirror.example.test"))
	b := lock(pk("core", "1.0.0", "1", "mirror.example.test"), pk("odd", "1.0.0", "2", "mirror.example.test"), pk("newbie", "0.1.0", "3", "evil.example.test"))
	expect(t, "sources", a, b, pol, []fk{
		{"core", "source-changed", "high"}, {"core", "source-not-allowed", "high"},
		{"newbie", "added", "info"}, {"newbie", "source-not-allowed", "high"},
		{"odd", "source-not-allowed", "high"},
	})
	allow := Policy{AllowedSources: map[string]bool{R: true, "mirror.example.test": true}}
	expect(t, "mirror allowed", a, b, allow, []fk{
		{"core", "source-changed", "high"}, {"newbie", "added", "info"}, {"newbie", "source-not-allowed", "high"},
	})
}

func TestCombinedFindingsKeepKindOrder(t *testing.T) {
	a := lock(pk("zeta", "2.0.0", "1", R))
	b := lock(pk("zeta", "1.0.0", "2", "evil.example.test"))
	expect(t, "downgrade from a bad host", a, b, pol, []fk{
		{"zeta", "downgrade", "high"}, {"zeta", "source-changed", "high"}, {"zeta", "source-not-allowed", "high"},
	})
}

func TestLookalikes(t *testing.T) {
	a := lock(pk("requests", "2.0.0", "1", R), pk("abc", "1.0.0", "2", R), pk("left-pad", "1.0.0", "3", R))
	cases := []struct {
		name string
		want bool
		det  string
	}{
		{"reqeusts", false, ""}, // transposition is distance 2
		{"request", true, "requests"},
		{"requestss", true, "requests"},
		{"reques7s", true, "requests"},
		{"Requests", true, "requests"},
		{"left_pad", true, "left-pad"},
		{"leftpad", true, "left-pad"},
		{"abd", false, ""}, // under 4 characters
		{"abcd", false, ""},
		{"httpz", true, "httpx"},
		{"httpx2", true, "httpx"},
		{"serde_lite", true, "serde-lite"},
		{"totally-new", false, ""},
	}
	for _, c := range cases {
		b := a + pk(c.name, "0.1.0", "9", R) + "\n"
		fs, err := Review(a, b, pol)
		if err != nil {
			t.Fatal(err)
		}
		var got *Finding
		for i := range fs {
			if fs[i].Kind == "lookalike" {
				got = &fs[i]
			}
		}
		if (got != nil) != c.want {
			t.Errorf("%q: lookalike reported = %v, want %v (%v)", c.name, got != nil, c.want, fs)
			continue
		}
		if got != nil && (got.Pkg != c.name || got.Severity != "high" || got.Detail != "resembles "+c.det) {
			t.Errorf("%q: finding %+v, want detail %q", c.name, *got, "resembles "+c.det)
		}
	}
}

func TestExistingNamesAreNotLookalikes(t *testing.T) {
	// requests and request both already existed: neither is newly added.
	a := lock(pk("requests", "2.0.0", "1", R), pk("request", "1.0.0", "2", R))
	b := lock(pk("requests", "2.0.1", "3", R), pk("request", "1.0.0", "2", R))
	expect(t, "already present", a, b, pol, []fk{{"requests", "upgrade", "info"}})
}

func TestFirstLookalikeMatchIsAlphabetical(t *testing.T) {
	a := lock(pk("tokio-a", "1.0.0", "1", R), pk("tokio-b", "1.0.0", "2", R))
	b := a + pk("tokio-c", "0.1.0", "3", R) + "\n"
	fs, _ := Review(a, b, pol)
	for _, f := range fs {
		if f.Kind == "lookalike" && f.Detail != "resembles tokio-a" {
			t.Errorf("detail = %q, want resembles tokio-a", f.Detail)
		}
	}
}

func TestMalformedInputIsAnError(t *testing.T) {
	ok := lock(pk("a1b2", "1.0.0", "1", R))
	bads := []string{
		lock("pkg only three fields"),
		lock(pk("x", "1.0", "1", R)),
		lock(pk("x", "01.0.0", "1", R)),
		lock(pk("x", "1.0.0", "A", R)),
		lock(pk("x", "1.0.0", "1", R), pk("x", "1.0.1", "2", R)),
		lock("pkg x 1.0.0 sha256:abc " + R),
	}
	for i, b := range bads {
		if _, err := Review(ok, b, pol); err == nil {
			t.Errorf("bad new lockfile %d accepted", i)
		}
		if _, err := Review(b, ok, pol); err == nil {
			t.Errorf("bad old lockfile %d accepted", i)
		}
	}
}

func TestRealisticReview(t *testing.T) {
	a := lock(pk("httpx", "0.9.4", "1", R), pk("logr", "1.4.2", "2", R), pk("yamlish", "3.0.0", "3", R), pk("tinycsv", "0.2.0", "4", R))
	b := lock(pk("httpx", "0.9.4", "f", R), pk("logr", "1.5.0", "5", R), pk("yamlish", "2.9.9", "6", R),
		pk("tinycsv", "0.2.0", "4", R), pk("log-r", "0.0.1", "7", "pkgs.example.test"), pk("telemetry-helper", "1.0.0", "8", R))
	expect(t, "release review", a, b, pol, []fk{
		{"httpx", "hash-changed", "critical"},
		{"log-r", "added", "info"}, {"log-r", "source-not-allowed", "high"}, {"log-r", "lookalike", "high"},
		{"logr", "upgrade", "info"},
		{"telemetry-helper", "added", "info"},
		{"yamlish", "downgrade", "high"},
	})
}
