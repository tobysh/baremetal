package main

import (
	"bytes"
	"reflect"
	"testing"
)

func TestDirectory(t *testing.T) {
	v := []DirEntry{{"a", 1}, {"twelve-chars", 19}, {"z", 0x12345678}}
	want := make([]byte, 48)
	copy(want, "a")
	want[15] = 1
	copy(want[16:], "twelve-chars")
	want[31] = 19
	want[32] = 'z'
	copy(want[44:], []byte{0x12, 0x34, 0x56, 0x78})
	b, e := EncodeDir(v)
	if e != nil || !bytes.Equal(b, want) {
		t.Fatal("encoding", b, e)
	}
	got, e := DecodeDir(want)
	if e != nil || !reflect.DeepEqual(got, v) {
		t.Fatal(got, e)
	}
	for _, bad := range [][]DirEntry{{{"", 1}}, {{"a/b", 1}}, {{"x", 0}}, {{"x", 1}, {"x", 2}}, {{"1234567890123", 1}}, {{"é", 1}}} {
		if out, e := EncodeDir(bad); e == nil || out != nil {
			t.Fatal("invalid name")
		}
	}
	for _, off := range []int{0, 2, 12, 15} {
		bad := append([]byte(nil), want[:16]...)
		switch off {
		case 0:
			bad[0] = '/'
		case 2:
			bad[2] = 'x'
		case 12:
			bad[0] = 0
		case 15:
			bad[15] = 0
		}
		if out, e := DecodeDir(bad); e == nil || out != nil {
			t.Fatal("corruption", off)
		}
	}
	if _, e := DecodeDir(want[:47]); e == nil {
		t.Fatal("truncation")
	}
	if _, e := DecodeDir(append(want[:16:16], want[:16]...)); e == nil {
		t.Fatal("duplicate")
	}
}

func TestDirectoryVariedRecords(t *testing.T) {
	for n := 0; n < 40; n++ {
		entries := make([]DirEntry, n)
		want := make([]byte, n*16)
		for i := range entries {
			name := string([]byte{'f', byte('A' + i/26), byte('a' + i%26)})
			ino := uint32(1 + i*257 + n*65536)
			entries[i] = DirEntry{name, ino}
			copy(want[i*16:i*16+12], name)
			for j := 0; j < 4; j++ {
				want[i*16+12+j] = byte(ino >> uint(24-8*j))
			}
		}
		got, e := EncodeDir(entries)
		if e != nil || !bytes.Equal(got, want) {
			t.Fatal("varied encoding", n, e)
		}
		decoded, e := DecodeDir(want)
		if e != nil || len(decoded) != len(entries) {
			t.Fatal("varied decoding", n, e)
		}
		for i := range entries {
			if decoded[i] != entries[i] {
				t.Fatal("varied record", n, i)
			}
		}
	}
}
