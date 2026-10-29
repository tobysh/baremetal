package main

import (
	"bytes"
	"encoding/binary"
	"errors"
	"fmt"
	"hash/crc32"
	"testing"
)

func payloads(n int) [][]byte {
	var ps [][]byte
	for i := 0; i < n; i++ {
		ps = append(ps, []byte(fmt.Sprintf("payload number %d %s", i, bytes.Repeat([]byte("x"), i*3))))
	}
	return ps
}

func joinFrames(ps [][]byte) ([]byte, []int) {
	var log []byte
	var ends []int
	for _, p := range ps {
		log = append(log, Frame(p)...)
		ends = append(ends, len(log))
	}
	return log, ends
}

func TestFrame_layout(t *testing.T) {
	f := Frame([]byte("abc"))
	if len(f) != 11 {
		t.Fatalf("len(Frame(\"abc\")) = %d; want 11", len(f))
	}
	if binary.BigEndian.Uint32(f[4:8]) != 3 || string(f[8:]) != "abc" {
		t.Errorf("length or payload is in the wrong place: % x", f)
	}
	want := crc32.ChecksumIEEE(append([]byte{0, 0, 0, 3}, "abc"...))
	if got := binary.BigEndian.Uint32(f[0:4]); got != want {
		t.Errorf("checksum = %08x; want %08x (CRC-32 IEEE over the length bytes and the payload)", got, want)
	}
}

func TestUnframe_round_trip_with_empty_payload(t *testing.T) {
	for _, p := range [][]byte{[]byte("hello"), {}, bytes.Repeat([]byte{0xAB}, 1000)} {
		f := append(Frame(p), "trailing bytes"...)
		got, n, err := Unframe(f)
		if err != nil || !bytes.Equal(got, p) || n != 8+len(p) {
			t.Errorf("Unframe(Frame(%d bytes)) = %d bytes, n=%d, err=%v", len(p), len(got), n, err)
		}
	}
}

func TestUnframe_short_input(t *testing.T) {
	f := Frame([]byte("some payload"))
	for cut := 0; cut < len(f); cut++ {
		if _, _, err := Unframe(f[:cut]); !errors.Is(err, ErrShort) {
			t.Errorf("frame cut at %d of %d bytes: err = %v; want ErrShort", cut, len(f), err)
		}
	}
}

func TestUnframe_detects_every_flipped_bit_in_the_payload(t *testing.T) {
	f := Frame([]byte("precious data"))
	for i := 8; i < len(f); i++ {
		for bit := 0; bit < 8; bit++ {
			bad := append([]byte{}, f...)
			bad[i] ^= 1 << bit
			if _, _, err := Unframe(bad); !errors.Is(err, ErrCorrupt) {
				t.Fatalf("bit %d of byte %d flipped: err = %v; want ErrCorrupt", bit, i, err)
			}
		}
	}
}

func TestUnframe_detects_a_damaged_checksum(t *testing.T) {
	f := Frame([]byte("precious data"))
	f[2] ^= 0x10
	if _, _, err := Unframe(f); !errors.Is(err, ErrCorrupt) {
		t.Errorf("damaged checksum: err = %v; want ErrCorrupt", err)
	}
}

func TestUnframe_huge_length_does_not_panic(t *testing.T) {
	f := Frame([]byte("abc"))
	binary.BigEndian.PutUint32(f[4:8], 0xFFFFFFFF)
	_, _, err := Unframe(f)
	if err == nil {
		t.Errorf("a frame claiming 4 GB was accepted")
	}
}

func TestScanFrames_clean_log(t *testing.T) {
	ps := payloads(30)
	log, _ := joinFrames(ps)
	got, valid, err := ScanFrames(log)
	if err != nil || valid != len(log) || len(got) != len(ps) {
		t.Fatalf("clean log: %d payloads, valid=%d/%d, err=%v", len(got), valid, len(log), err)
	}
	for i := range ps {
		if !bytes.Equal(got[i], ps[i]) {
			t.Errorf("payload %d = %q; want %q", i, got[i], ps[i])
		}
	}
	if got, valid, err := ScanFrames(nil); len(got) != 0 || valid != 0 || err != nil {
		t.Errorf("empty log = %v, %d, %v", got, valid, err)
	}
}

func TestScanFrames_torn_tail_is_not_an_error(t *testing.T) {
	ps := payloads(6)
	log, ends := joinFrames(ps)
	for cut := 0; cut <= len(log); cut++ {
		complete, good := 0, 0
		for i, e := range ends {
			if e <= cut {
				complete, good = i+1, e
			}
		}
		got, valid, err := ScanFrames(log[:cut])
		if err != nil {
			t.Fatalf("log cut at %d: err = %v; a torn tail must not be an error", cut, err)
		}
		if len(got) != complete || valid != good {
			t.Fatalf("log cut at %d: %d frames, valid=%d; want %d frames, valid=%d", cut, len(got), valid, complete, good)
		}
	}
}

func TestScanFrames_corruption_stops_the_scan(t *testing.T) {
	ps := payloads(6)
	log, ends := joinFrames(ps)
	// Flip a payload byte of frame index 3.
	bad := append([]byte{}, log...)
	bad[ends[2]+10] ^= 0x40
	got, valid, err := ScanFrames(bad)
	if !errors.Is(err, ErrCorrupt) {
		t.Fatalf("err = %v; want an error wrapping ErrCorrupt", err)
	}
	if len(got) != 3 || valid != ends[2] {
		t.Errorf("good frames = %d, valid = %d; want 3 and %d", len(got), valid, ends[2])
	}
}

func TestScanFrames_damaged_length_never_yields_wrong_data(t *testing.T) {
	ps := payloads(6)
	log, ends := joinFrames(ps)
	for b := 4; b < 8; b++ {
		bad := append([]byte{}, log...)
		pos := ends[1] + b
		bad[pos] ^= 0x01
		got, valid, err := ScanFrames(bad)
		for i, p := range got {
			if !bytes.Equal(p, ps[i]) {
				t.Errorf("length byte %d damaged: frame %d came back wrong", b, i)
			}
		}
		if err == nil && len(got) == len(ps) {
			t.Errorf("length byte %d damaged but the whole log scanned clean", b)
		}
		if valid > ends[1] {
			t.Errorf("length byte %d damaged: valid = %d runs past the damage at %d", b, valid, ends[1])
		}
	}
}
