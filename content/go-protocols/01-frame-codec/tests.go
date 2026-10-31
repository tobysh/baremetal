package main

import (
	"bytes"
	"errors"
	"hash/crc32"
	"math/rand"
	"testing"
)

func refFrame(typ byte, p []byte) []byte {
	b := []byte{0xBE, typ, byte(len(p) >> 8), byte(len(p))}
	b = append(b, p...)
	s := crc32.ChecksumIEEE(b)
	return append(b, byte(s>>24), byte(s>>16), byte(s>>8), byte(s))
}

func refPayload(r *rand.Rand, n int) []byte {
	p := make([]byte, n)
	r.Read(p)
	return p
}

func TestEncode_golden_vector(t *testing.T) {
	// Independent of any code: type 7, payload "hi", CRC32 b9ece38b.
	want := []byte{0xbe, 0x07, 0x00, 0x02, 'h', 'i', 0xb9, 0xec, 0xe3, 0x8b}
	got, err := Encode(7, []byte("hi"))
	if err != nil || !bytes.Equal(got, want) {
		t.Errorf("Encode(7, hi) = % x, %v; want % x", got, err, want)
	}
}

func TestEncode_matches_reference(t *testing.T) {
	r := rand.New(rand.NewSource(1))
	for _, n := range []int{0, 1, 2, 3, 255, 256, 257, 1023, 1024} {
		p := refPayload(r, n)
		typ := byte(r.Intn(256))
		got, err := Encode(typ, p)
		if err != nil || !bytes.Equal(got, refFrame(typ, p)) {
			t.Errorf("Encode with %d byte payload differs from the format (err %v)", n, err)
		}
	}
}

func TestEncode_too_large(t *testing.T) {
	b, err := Encode(1, make([]byte, MaxPayload+1))
	if !errors.Is(err, ErrTooLarge) || b != nil {
		t.Errorf("1025 byte payload: %v, %v; want nil, ErrTooLarge", b, err)
	}
}

func TestDecode_round_trip(t *testing.T) {
	r := rand.New(rand.NewSource(2))
	for i := 0; i < 60; i++ {
		p := refPayload(r, r.Intn(MaxPayload+1))
		typ := byte(r.Intn(256))
		f := refFrame(typ, p)
		gt, gp, n, err := Decode(f)
		if err != nil || gt != typ || !bytes.Equal(gp, p) || n != len(f) {
			t.Fatalf("round trip %d: got type %d len %d n %d err %v; want type %d len %d n %d", i, gt, len(gp), n, err, typ, len(p), len(f))
		}
	}
}

func TestDecode_every_prefix_is_short(t *testing.T) {
	for _, n := range []int{0, 1, 17, 300} {
		f := refFrame(9, bytes.Repeat([]byte{0x5a}, n))
		for cut := 0; cut < len(f); cut++ {
			_, _, used, err := Decode(f[:cut])
			if !errors.Is(err, ErrShort) || used != 0 {
				t.Fatalf("payload %d, cut at %d of %d: n=%d err=%v; want 0, ErrShort", n, cut, len(f), used, err)
			}
		}
	}
}

func TestDecode_every_bit_flip_is_detected(t *testing.T) {
	f := refFrame(3, []byte("the quick brown fox"))
	for pos := range f {
		for bit := 0; bit < 8; bit++ {
			g := append([]byte(nil), f...)
			g[pos] ^= 1 << bit
			_, _, n, err := Decode(g)
			if err == nil || n != 0 {
				t.Fatalf("flip byte %d bit %d went undetected (n=%d)", pos, bit, n)
			}
			switch {
			case pos == 0 && !errors.Is(err, ErrMagic):
				t.Fatalf("flip in magic: err %v; want ErrMagic", err)
			case (pos == 1 || pos >= 4) && !errors.Is(err, ErrChecksum):
				t.Fatalf("flip byte %d: err %v; want ErrChecksum", pos, err)
			}
		}
	}
}

func TestDecode_early_rejection(t *testing.T) {
	if _, _, _, err := Decode([]byte{0x00}); !errors.Is(err, ErrMagic) {
		t.Errorf("one wrong byte: %v; want ErrMagic", err)
	}
	if _, _, _, err := Decode([]byte{0xBE, 1, 0x04, 0x01}); !errors.Is(err, ErrTooLarge) {
		t.Errorf("announced 1025 bytes: %v; want ErrTooLarge with only the header present", err)
	}
	if _, _, _, err := Decode([]byte{0xBE, 1, 0xff, 0xff, 0}); !errors.Is(err, ErrTooLarge) {
		t.Errorf("announced 65535 bytes: %v; want ErrTooLarge", err)
	}
	if _, _, _, err := Decode([]byte{0xBE, 1, 0x04}); !errors.Is(err, ErrShort) {
		t.Errorf("3 header bytes: %v; want ErrShort", err)
	}
}

func TestDecode_leaves_trailing_bytes(t *testing.T) {
	a := refFrame(1, []byte("first"))
	b := refFrame(2, []byte("second"))
	buf := append(append([]byte(nil), a...), b...)
	typ, p, n, err := Decode(buf)
	if err != nil || typ != 1 || string(p) != "first" || n != len(a) {
		t.Fatalf("first: %d %q %d %v", typ, p, n, err)
	}
	typ, p, n, err = Decode(buf[n:])
	if err != nil || typ != 2 || string(p) != "second" || n != len(b) {
		t.Fatalf("second: %d %q %d %v", typ, p, n, err)
	}
}

func TestDecode_payload_is_a_copy(t *testing.T) {
	f := refFrame(1, []byte("hello"))
	_, p, _, _ := Decode(f)
	copy(f, bytes.Repeat([]byte{0}, len(f)))
	if string(p) != "hello" {
		t.Errorf("payload changed to %q when the input buffer was reused", p)
	}
}
