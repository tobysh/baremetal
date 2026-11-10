package main

import (
	"bytes"
	"reflect"
	"strings"
	"testing"
)

func rec(name string, ino uint32) []byte {
	b := make([]byte, 16)
	copy(b, name)
	b[12], b[13], b[14], b[15] = byte(ino>>24), byte(ino>>16), byte(ino>>8), byte(ino)
	return b
}

func TestExactBytes(t *testing.T) {
	v := []DirEntry{{"a", 1}, {"twelve-chars", 19}, {"z", 0x12345678}}
	want := append(append(rec("a", 1), rec("twelve-chars", 19)...), rec("z", 0x12345678)...)
	b, e := EncodeDir(v)
	if e != nil || !bytes.Equal(b, want) {
		t.Fatalf("EncodeDir = %x, %v\nwant        %x", b, e, want)
	}
	got, e := DecodeDir(want)
	if e != nil || !reflect.DeepEqual(got, v) {
		t.Fatalf("DecodeDir = %v, %v; want %v", got, e, v)
	}
}

func TestEmptyDirectory(t *testing.T) {
	b, e := EncodeDir(nil)
	if e != nil || len(b) != 0 {
		t.Fatalf("EncodeDir(nil) = %x, %v; want zero bytes and no error", b, e)
	}
	b, e = EncodeDir([]DirEntry{})
	if e != nil || len(b) != 0 {
		t.Fatalf("EncodeDir(empty) = %x, %v", b, e)
	}
	got, e := DecodeDir(nil)
	if e != nil || len(got) != 0 {
		t.Fatalf("DecodeDir(nil) = %v, %v; an empty directory is valid", got, e)
	}
	got, e = DecodeDir([]byte{})
	if e != nil || len(got) != 0 {
		t.Fatalf("DecodeDir(empty) = %v, %v", got, e)
	}
}

func TestValidNamesAccepted(t *testing.T) {
	for _, name := range []string{"a", "A", "123456789012", ".hidden", "a.b", "..", ".", "has space", "UPPER", "x-y_z"} {
		v := []DirEntry{{name, 0xffffffff}}
		b, e := EncodeDir(v)
		if e != nil || len(b) != 16 {
			t.Fatalf("name %q must be valid: %v", name, e)
		}
		got, e := DecodeDir(b)
		if e != nil || !reflect.DeepEqual(got, v) {
			t.Fatalf("name %q did not round-trip: %v, %v", name, got, e)
		}
	}
	// Case sensitive, so these are different names.
	if _, e := EncodeDir([]DirEntry{{"a", 1}, {"A", 2}}); e != nil {
		t.Fatalf("names differing in case are distinct: %v", e)
	}
	// The same inode under several names is allowed (hard links).
	if _, e := EncodeDir([]DirEntry{{"a", 7}, {"b", 7}}); e != nil {
		t.Fatalf("two names for one inode are valid: %v", e)
	}
}

func TestEncodeRejectsEachRule(t *testing.T) {
	cases := []struct {
		why string
		in  []DirEntry
	}{
		{"empty name", []DirEntry{{"", 1}}},
		{"slash", []DirEntry{{"a/b", 1}}},
		{"only a slash", []DirEntry{{"/", 1}}},
		{"NUL inside", []DirEntry{{"a\x00b", 1}}},
		{"13 bytes", []DirEntry{{"1234567890123", 1}}},
		{"non-ASCII", []DirEntry{{"\u00e9", 1}}},
		{"high byte", []DirEntry{{"a\x80", 1}}},
		{"inode zero", []DirEntry{{"x", 0}}},
		{"duplicate adjacent", []DirEntry{{"x", 1}, {"x", 2}}},
		{"duplicate far apart", []DirEntry{{"x", 1}, {"y", 2}, {"z", 3}, {"x", 4}}},
		{"bad entry last", []DirEntry{{"ok", 1}, {"ok2", 2}, {"", 3}}},
	}
	for _, c := range cases {
		if out, e := EncodeDir(c.in); e == nil || out != nil {
			t.Fatalf("%s: EncodeDir returned %x, %v; want nil and an error", c.why, out, e)
		}
	}
}

func TestDecodeRejectsEachRule(t *testing.T) {
	good := rec("name", 5)
	mutate := func(f func(b []byte)) []byte {
		b := append([]byte(nil), good...)
		f(b)
		return b
	}
	cases := []struct {
		why  string
		data []byte
	}{
		{"1 byte", good[:1]},
		{"15 bytes", good[:15]},
		{"17 bytes", append(append([]byte(nil), good...), 0)},
		{"47 of 48 bytes", append(append(rec("a", 1), rec("b", 2)...), rec("c", 3)[:15]...)},
		{"empty name", mutate(func(b []byte) { b[0] = 0 })},
		{"garbage after NUL", mutate(func(b []byte) { b[7] = 'x' })},
		{"last name byte after NUL", mutate(func(b []byte) { b[11] = 1 })},
		{"slash", mutate(func(b []byte) { b[1] = '/' })},
		{"high byte", mutate(func(b []byte) { b[0] = 0xc3 })},
		{"inode zero", mutate(func(b []byte) { b[12], b[13], b[14], b[15] = 0, 0, 0, 0 })},
		{"duplicate", append(append(rec("dup", 1), rec("other", 2)...), rec("dup", 3)...)},
		{"valid then bad", append(rec("fine", 1), mutate(func(b []byte) { b[9] = 'q' })...)},
	}
	for _, c := range cases {
		if out, e := DecodeDir(c.data); e == nil || out != nil {
			t.Fatalf("%s: DecodeDir returned %v, %v; want nil and an error", c.why, out, e)
		}
	}
}

func TestVariedRecords(t *testing.T) {
	for n := 0; n < 40; n++ {
		entries := make([]DirEntry, n)
		want := make([]byte, 0, n*16)
		for i := range entries {
			name := string([]byte{'f', byte('A' + i/26), byte('a' + i%26)})
			if i%3 == 0 {
				name += strings.Repeat("x", i%10)
			}
			ino := uint32(1 + i*257 + n*65536)
			entries[i] = DirEntry{name, ino}
			want = append(want, rec(name, ino)...)
		}
		got, e := EncodeDir(entries)
		if e != nil || !bytes.Equal(got, want) {
			t.Fatalf("n=%d: EncodeDir mismatch (%v)", n, e)
		}
		decoded, e := DecodeDir(want)
		if e != nil || len(decoded) != len(entries) {
			t.Fatalf("n=%d: DecodeDir: %v, %d entries", n, e, len(decoded))
		}
		for i := range entries {
			if decoded[i] != entries[i] {
				t.Fatalf("n=%d record %d: got %v, want %v (order must be preserved)", n, i, decoded[i], entries[i])
			}
		}
	}
}

func TestInputsNotModified(t *testing.T) {
	entries := []DirEntry{{"b", 2}, {"a", 1}, {"c", 3}}
	copyOf := append([]DirEntry(nil), entries...)
	data, e0 := EncodeDir(entries)
	if e0 != nil || len(data) != 48 {
		t.Fatalf("EncodeDir = %x, %v", data, e0)
	}
	if !reflect.DeepEqual(entries, copyOf) {
		t.Fatal("EncodeDir reordered or modified its input")
	}
	if data[0] != 'b' {
		t.Fatal("EncodeDir must keep the given order, not sort")
	}
	snapshot := append([]byte(nil), data...)
	out, e := DecodeDir(data)
	if e != nil || len(out) != 3 {
		t.Fatalf("DecodeDir = %v, %v", out, e)
	}
	if !bytes.Equal(data, snapshot) {
		t.Fatal("DecodeDir modified its input")
	}
	out[0].Name = "changed"
	if data[0] != 'b' {
		t.Fatal("DecodeDir result must not alias its input")
	}
	// A failed decode must not modify the input either.
	bad := append(append([]byte(nil), data...), 1)
	snapshot = append([]byte(nil), bad...)
	DecodeDir(bad)
	if !bytes.Equal(bad, snapshot) {
		t.Fatal("a failed DecodeDir modified its input")
	}
}
