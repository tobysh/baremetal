package main

import (
	"bytes"
	"encoding/binary"
	"hash/crc32"
	"math/rand"
	"testing"
)

func TestCapsuleFields(t *testing.T) {
	rng := rand.New(rand.NewSource(2311))
	for _, n := range []int{1, 2, 17, 255, 256, 4096} {
		code := make([]byte, n)
		rng.Read(code)
		before := append([]byte{}, code...)
		entry := uint16(n - 1)
		got, err := Wrap(code, entry)
		if err != nil || len(got) != 12+n {
			t.Fatal(n, len(got), err)
		}
		if string(got[:4]) != "BML1" || binary.LittleEndian.Uint16(got[4:6]) != entry || int(binary.LittleEndian.Uint16(got[6:8])) != n || binary.LittleEndian.Uint32(got[8:12]) != crc32.ChecksumIEEE(code) || !bytes.Equal(got[12:], code) || !bytes.Equal(code, before) {
			t.Fatal("wrong fields", n)
		}
		code[0] ^= 255
		if !bytes.Equal(got[12:], before) {
			t.Fatal("capsule aliases input")
		}
		got[12] ^= 127
		if code[0] == got[12] {
			t.Fatal("unexpected alias")
		}
	}
}
func TestCapsuleRanges(t *testing.T) {
	for _, c := range []struct {
		code  []byte
		entry uint16
	}{{nil, 0}, {[]byte{0}, 1}, {make([]byte, 4097), 0}, {make([]byte, 12), 65535}} {
		if out, err := Wrap(c.code, c.entry); err == nil || out != nil {
			t.Fatal("invalid range accepted")
		}
	}
}
func TestCapsuleOwnStorage(t *testing.T) {
	a, _ := Wrap([]byte{0}, 0)
	b, _ := Wrap([]byte{0}, 0)
	a[12] = 17
	if b[12] != 0 {
		t.Fatal("calls share output")
	}
}
