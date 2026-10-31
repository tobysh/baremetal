package main

import (
	"bytes"
	"encoding/base64"
	"encoding/hex"
	"fmt"
	"reflect"
	"strings"
	"testing"
)

// rec builds a record from a version and raw body, independent of Encode.
func rec(ver byte, body ...byte) []byte {
	out := []byte{'B', 'C', 'F', 'G', ver, byte(len(body)), byte(len(body) >> 8)}
	return append(out, body...)
}

func field(tag byte, v ...byte) []byte { return append([]byte{tag, byte(len(v))}, v...) }

func cat(parts ...[]byte) []byte { return bytes.Join(parts, nil) }

var label = field(1, 'l', 'a', 'b')

// Golden records written by hand; they must keep decoding in every future version.
var goldenV1 = []byte{0x42, 0x43, 0x46, 0x47, 1, 5, 0, 1, 3, 'l', 'a', 'b'}
var goldenV1Full = []byte{0x42, 0x43, 0x46, 0x47, 1, 13, 0, 1, 3, 'l', 'a', 'b', 2, 2, 0x2c, 0x01, 3, 2, 0x00, 0x04}
var tok = "lab-token-1"
var goldenV2 = cat([]byte{'B', 'C', 'F', 'G', 2, 25, 0}, label, field(4, 2, 1, 'a', 1, 'b'), field(5, []byte(tok)...))

func TestDecodeGolden(t *testing.T) {
	want := map[string]Config{
		"v1":  {Label: "lab", Interval: 10, MaxOps: 64, Allow: []string{}},
		"v1f": {Label: "lab", Interval: 300, MaxOps: 1024, Allow: []string{}},
		"v2":  {Label: "lab", Interval: 10, MaxOps: 64, Allow: []string{"a", "b"}, Token: tok},
	}
	for name, b := range map[string][]byte{"v1": goldenV1, "v1f": goldenV1Full, "v2": goldenV2} {
		before := append([]byte{}, b...)
		got, err := Decode(b)
		if err != nil || !reflect.DeepEqual(got, want[name]) || !bytes.Equal(b, before) {
			t.Fatal(name, got, err)
		}
		for i := range b {
			b[i] = 'Z'
		}
		if got.Label != want[name].Label || got.Token != want[name].Token {
			t.Fatal("decoded strings alias input")
		}
		copy(b, before)
	}
	if got, _ := Decode(goldenV2); got.Allow == nil {
		t.Fatal("Allow must be non-nil")
	}
}

func TestEncodeGolden(t *testing.T) {
	got, err := Encode(Config{Label: "lab", Interval: 10, MaxOps: 64})
	if err != nil || !bytes.Equal(got, goldenV1) {
		t.Fatalf("%x %v", got, err)
	}
	got, err = Encode(Config{Label: "lab", Interval: 300, MaxOps: 1024})
	if err != nil || !bytes.Equal(got, goldenV1Full) {
		t.Fatalf("%x %v", got, err)
	}
	got, err = Encode(Config{Label: "lab", Interval: 10, MaxOps: 64, Allow: []string{"a", "b"}, Token: tok})
	if err != nil || !bytes.Equal(got, goldenV2) {
		t.Fatalf("%x %v", got, err)
	}
	// Token alone still needs version 2.
	got, _ = Encode(Config{Label: "lab", Interval: 10, MaxOps: 64, Token: "t"})
	if len(got) < 5 || got[4] != 2 {
		t.Fatalf("token needs v2: %x", got)
	}
}

func TestRoundTripsVaryFields(t *testing.T) {
	seed := uint32(12345)
	next := func(n int) int { seed = seed*1664525 + 1013904223; return int(seed>>8) % n }
	str := func(min, max int) string {
		n := min + next(max-min+1)
		b := make([]byte, n)
		for i := range b {
			b[i] = byte(next(256))
		}
		return string(b)
	}
	for i := 0; i < 300; i++ {
		c := Config{Label: str(1, 32), Interval: uint16(1 + next(65535)), MaxOps: uint16(1 + next(4096)), Allow: []string{}}
		if next(3) == 0 {
			c.Interval = 10
		}
		if next(3) == 0 {
			c.MaxOps = 64
		}
		for k := next(9); k > 0; k-- {
			key := fmt.Sprintf("%d:%s", len(c.Allow), str(0, 20))
			c.Allow = append(c.Allow, key)
		}
		if next(2) == 0 {
			c.Token = str(1, 64)
		}
		b, err := Encode(c)
		if err != nil {
			t.Fatal(c, err)
		}
		got, err := Decode(b)
		if err != nil || !reflect.DeepEqual(got, c) {
			t.Fatalf("round trip %v -> %x -> %v %v", c, b, got, err)
		}
		again, _ := Encode(got)
		if !bytes.Equal(again, b) {
			t.Fatal("encoding not canonical")
		}
		wantVer := byte(1)
		if len(c.Allow) > 0 || c.Token != "" {
			wantVer = 2
		}
		if b[4] != wantVer {
			t.Fatal("not the lowest version", b[4], wantVer)
		}
		// Every strict prefix is an error.
		for cut := 0; cut < len(b); cut++ {
			if g, err := Decode(b[:cut]); err == nil || !reflect.DeepEqual(g, Config{}) {
				t.Fatal("truncation accepted at", cut)
			}
		}
	}
}

func TestTruncatedFieldsWithConsistentLength(t *testing.T) {
	// The outer length is right but the last field is cut short.
	bodies := [][]byte{
		{1}, {1, 3, 'l', 'a'}, cat(label, []byte{2}), cat(label, []byte{2, 2, 5}), cat(label, []byte{3, 2, 5}),
		cat(label, []byte{4, 3, 2, 1, 'a'}), cat(label, []byte{4, 5, 1, 4, 'a'}), cat(label, []byte{0x90, 4, 1}), cat(label, []byte{0x90}),
	}
	for i, b := range bodies {
		if g, err := Decode(rec(2, b...)); err == nil || !reflect.DeepEqual(g, Config{}) {
			t.Fatal("accepted", i)
		}
	}
	// Whole-record mismatches.
	good := rec(1, label...)
	for name, b := range map[string][]byte{"trailing": append(append([]byte{}, good...), 0), "short": good[:len(good)-1], "magic": append([]byte("BCFX"), good[4:]...), "empty": nil, "header only": good[:7]} {
		if _, err := Decode(b); err == nil {
			t.Fatal(name)
		}
	}
	long := rec(1, label...)
	long[5], long[6] = 0x01, 0x04 // claims 1025 bytes
	long = append(long, make([]byte, 1025-len(label))...)
	if _, err := Decode(long); err == nil {
		t.Fatal("1025-byte body accepted")
	}
}

func TestVersionAndUnknownFieldCompatibility(t *testing.T) {
	// Unknown ignorable fields are skipped anywhere, including before the label.
	for _, ver := range []byte{1, 2} {
		for _, junk := range [][]byte{field(0x80), field(0xff, 1, 2, 3), field(0x80, bytes.Repeat([]byte{9}, 255)...)} {
			for _, b := range [][]byte{cat(junk, label), cat(label, junk), cat(junk, label, junk, field(2, 5, 0))} {
				got, err := Decode(rec(ver, b...))
				if err != nil || got.Label != "lab" {
					t.Fatal("ignorable field rejected", ver, err)
				}
			}
		}
	}
	got, _ := Decode(rec(1, cat(label, field(0xa0, 1), field(2, 5, 0))...))
	if got.Interval != 5 {
		t.Fatal("field after ignorable one lost", got)
	}
	// Unknown critical tags, and v2 tags in a v1 record.
	for _, tag := range []byte{0, 6, 7, 0x10, 0x7f} {
		if _, err := Decode(rec(2, cat(label, field(tag, 1))...)); err == nil {
			t.Fatal("critical tag accepted", tag)
		}
	}
	for _, b := range [][]byte{field(4, 0), field(5, 'x')} {
		if _, err := Decode(rec(1, cat(label, b)...)); err == nil {
			t.Fatal("v2 tag accepted in v1 record")
		}
		if _, err := Decode(rec(2, cat(label, b)...)); err != nil {
			t.Fatal("v2 tag rejected in v2 record", err)
		}
	}
	for _, v := range []byte{0, 3, 9, 255} {
		if _, err := Decode(rec(v, label...)); err == nil {
			t.Fatal("version accepted", v)
		}
	}
	// A v1 record is also valid as a v2 record: older files stay readable.
	a, _ := Decode(rec(1, label...))
	b, _ := Decode(rec(2, label...))
	if !reflect.DeepEqual(a, b) {
		t.Fatal("v1 and v2 differ for v1 content")
	}
}

func TestInvalidValues(t *testing.T) {
	long33 := bytes.Repeat([]byte{'x'}, 33)
	cases := map[string][]byte{
		"no label": nil, "empty label": field(1), "long label": field(1, long33...), "dup label": cat(label, label),
		"zero interval": cat(label, field(2, 0, 0)), "short interval": cat(label, field(2, 5)), "long interval": cat(label, field(2, 5, 0, 0)),
		"dup interval": cat(label, field(2, 5, 0), field(2, 6, 0)),
		"zero ops":     cat(label, field(3, 0, 0)), "ops 4097": cat(label, field(3, 1, 0x10)), "ops 1 byte": cat(label, field(3, 5)),
		"allow empty value": cat(label, field(4)), "allow 9": cat(label, field(4, 9)), "allow dup": cat(label, field(4, 2, 1, 'a', 1, 'a')),
		"allow trailing": cat(label, field(4, 1, 1, 'a', 0)), "allow empty key": cat(label, field(4, 1, 0)), "allow missing key": cat(label, field(4, 1)),
		"allow key overrun": cat(label, field(4, 1, 5, 'a')), "allow 33-byte key": cat(label, field(4, append([]byte{1, 33}, long33...)...)),
		"token empty": cat(label, field(5)), "token 65": cat(label, field(5, bytes.Repeat([]byte{'t'}, 65)...)), "dup token": cat(label, field(5, 'a'), field(5, 'b')),
	}
	for name, body := range cases {
		if g, err := Decode(rec(2, body...)); err == nil || !reflect.DeepEqual(g, Config{}) {
			t.Fatal("accepted:", name, g)
		}
	}
	// Boundary values that are valid.
	ok := cat(field(1, long33[:32]...), field(2, 0xff, 0xff), field(3, 0, 0x10), field(5, bytes.Repeat([]byte{'t'}, 64)...))
	if g, err := Decode(rec(2, ok...)); err != nil || g.Interval != 65535 || g.MaxOps != 4096 || len(g.Token) != 64 {
		t.Fatal(g, err)
	}
	if g, err := Decode(rec(2, cat(label, field(4, 0))...)); err != nil || len(g.Allow) != 0 || g.Allow == nil {
		t.Fatal(g, err)
	}
	// Explicit defaults decode to the same Config as absent ones.
	g, err := Decode(rec(1, cat(label, field(2, 10, 0), field(3, 64, 0))...))
	if err != nil || g.Interval != 10 || g.MaxOps != 64 {
		t.Fatal(g, err)
	}
}

func TestEncodeRejectsInvalid(t *testing.T) {
	base := Config{Label: "lab", Interval: 10, MaxOps: 64}
	mod := func(f func(*Config)) Config { c := base; f(&c); return c }
	for name, c := range map[string]Config{
		"empty label": mod(func(c *Config) { c.Label = "" }), "long label": mod(func(c *Config) { c.Label = strings.Repeat("x", 33) }),
		"zero interval": mod(func(c *Config) { c.Interval = 0 }), "zero ops": mod(func(c *Config) { c.MaxOps = 0 }), "ops 4097": mod(func(c *Config) { c.MaxOps = 4097 }),
		"nine keys":  mod(func(c *Config) { c.Allow = []string{"1", "2", "3", "4", "5", "6", "7", "8", "9"} }),
		"dup keys":   mod(func(c *Config) { c.Allow = []string{"a", "a"} }),
		"empty key":  mod(func(c *Config) { c.Allow = []string{""} }),
		"long token": mod(func(c *Config) { c.Token = strings.Repeat("t", 65) }),
	} {
		if b, err := Encode(c); err == nil || b != nil {
			t.Fatal("accepted:", name)
		}
	}
	in := Config{Label: "lab", Interval: 10, MaxOps: 64, Allow: []string{"a"}}
	b, _ := Encode(in)
	b[len(b)-1] = 'Z'
	if in.Allow[0] != "a" {
		t.Fatal("input modified")
	}
}

func TestDescribeDoesNotLeakToken(t *testing.T) {
	for i := 0; i < 40; i++ {
		token := fmt.Sprintf("lab-secret-%02d-xyzzy", i)
		c := Config{Label: "lab-" + fmt.Sprint(i), Interval: uint16(i + 1), MaxOps: 64, Allow: []string{"a", "b"}, Token: token}
		d := Describe(c)
		forms := []string{token, hex.EncodeToString([]byte(token)), strings.ToUpper(hex.EncodeToString([]byte(token))), base64.StdEncoding.EncodeToString([]byte(token)), base64.RawURLEncoding.EncodeToString([]byte(token)), "xyzzy"}
		for k := 1; k < 256; k++ {
			x := []byte(token)
			for j := range x {
				x[j] ^= byte(k)
			}
			forms = append(forms, string(x))
		}
		for _, f := range forms {
			if strings.Contains(d, f) {
				t.Fatalf("Describe leaks %q: %s", f, d)
			}
		}
		want := fmt.Sprintf("label=lab-%d interval=%d maxops=64 allow=a,b token=set(len=%d)", i, i+1, len(token))
		if d != want {
			t.Fatalf("got %q want %q", d, want)
		}
	}
	if d := Describe(Config{Label: "x", Interval: 10, MaxOps: 64}); d != "label=x interval=10 maxops=64 allow= token=unset" {
		t.Fatal(d)
	}
	// The record itself carries the token as plain bytes: encoding is not protection.
	b, _ := Encode(Config{Label: "lab", Interval: 10, MaxOps: 64, Token: tok})
	if !bytes.Contains(b, []byte(tok)) {
		t.Fatal("expected the token to be visible in the record")
	}
}
