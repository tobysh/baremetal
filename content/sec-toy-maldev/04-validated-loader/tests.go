package main

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"hash/crc32"
	"reflect"
	"testing"
)

func capsuleFixture(code []byte, entry uint16) []byte {
	b := make([]byte, 12+len(code))
	copy(b, "BML1")
	binary.LittleEndian.PutUint16(b[4:6], entry)
	binary.LittleEndian.PutUint16(b[6:8], uint16(len(code)))
	binary.LittleEndian.PutUint32(b[8:12], crc32.ChecksumIEEE(code))
	copy(b[12:], code)
	return b
}
func TestLoaderEntryBoundaries(t *testing.T) {
	for i := 0; i < 20; i++ {
		text := fmt.Sprintf("m%d\x00", i)
		code := []byte{1, byte(len(text))}
		code = append(code, []byte(text)...)
		waitAt := uint16(len(code))
		code = append(code, 3, 2, 1, 0)
		for _, entry := range []uint16{0, waitAt, uint16(len(code) - 1)} {
			blob := capsuleFixture(code, entry)
			before := append([]byte{}, blob...)
			got, err := LoadCapsule(blob)
			idx := 0
			if entry == waitAt {
				idx = 1
			} else if entry > waitAt {
				idx = 2
			}
			want := Image{Instructions: []Instruction{{Op: OpEmit, Text: text}, {Op: OpWait, Ticks: 258}, {Op: OpHalt}}, Entry: idx}
			if err != nil || !reflect.DeepEqual(got, want) || !bytes.Equal(blob, before) {
				t.Fatal(entry, got, err)
			}
			blob[14] ^= 255
			if got.Instructions[0].Text != text {
				t.Fatal("loaded strings alias input")
			}
		}
	}
}
func TestLoaderRejectsMalformedInstructions(t *testing.T) {
	bad := [][]byte{{99, 0}, {1}, {1, 2, 'x'}, {2, 0, 0}, {3, 1}, {3, 0, 0, 0}, {0, 0}, {1, 1, 'x'}, {3, 1, 0}, {1, 255, 0}}
	many := []byte{}
	for i := 0; i < 64; i++ {
		many = append(many, 1, 0)
	}
	many = append(many, 0)
	bad = append(bad, many)
	for _, c := range bad {
		got, err := LoadCapsule(capsuleFixture(c, 0))
		if err == nil || !reflect.DeepEqual(got, Image{}) {
			t.Fatal("invalid instruction accepted", c, got)
		}
	}
}
func TestLoaderRejectsBadContainers(t *testing.T) {
	good := capsuleFixture([]byte{1, 2, 'h', 'i', 0}, 0)
	bad := [][]byte{nil, good[:11], good[:len(good)-1], append(append([]byte{}, good...), 0), capsuleFixture(nil, 0), capsuleFixture(make([]byte, 4097), 0), capsuleFixture([]byte{1, 2, 'h', 'i', 0}, 2), capsuleFixture([]byte{0}, 1)}
	for _, at := range []int{0, 6, 8, 14} {
		b := append([]byte{}, good...)
		b[at] ^= 1
		bad = append(bad, b)
	}
	for i, b := range bad {
		got, err := LoadCapsule(b)
		if err == nil || !reflect.DeepEqual(got, Image{}) {
			t.Fatal("bad container accepted", i)
		}
	}
}
func TestLoaderEmptyEmitAndHalt(t *testing.T) {
	got, err := LoadCapsule(capsuleFixture([]byte{1, 0, 0}, 2))
	if err != nil || got.Entry != 1 || len(got.Instructions) != 2 {
		t.Fatal(got, err)
	}
}
