package main

import (
	"bytes"
	"encoding/hex"
	"errors"
	"math"
	"runtime"
	"testing"
)

type tRand struct{ s uint64 }

func (r *tRand) next() uint64 {
	r.s = r.s*6364136223846793005 + 1442695040888963407
	return r.s >> 33
}
func (r *tRand) intn(n int) int { return int(r.next() % uint64(n)) }

func unhex(s string) []byte {
	b, err := hex.DecodeString(s)
	if err != nil {
		panic(err)
	}
	return b
}

// skewed returns n bytes drawn from a skewed distribution over `alpha` symbols, with runs.
func skewed(r *tRand, n, alpha int) []byte {
	out := make([]byte, 0, n)
	for len(out) < n {
		s := byte(r.intn(alpha) * r.intn(alpha) / alpha * 7 % 256)
		run := 1
		if r.intn(4) == 0 {
			run = 1 + r.intn(200)
		}
		for i := 0; i < run && len(out) < n; i++ {
			out = append(out, s)
		}
	}
	return out
}

func allocated(f func()) uint64 {
	var a, b runtime.MemStats
	runtime.GC()
	runtime.ReadMemStats(&a)
	f()
	runtime.ReadMemStats(&b)
	return b.TotalAlloc - a.TotalAlloc
}

var uvKnown = []uint64{0, 1, 127, 128, 300, 16383, 16384, 4294967295, 9223372036854775808, 18446744073709551615}
var uvEnc = []string{"00", "01", "7f", "8001", "ac02", "ff7f", "808001", "ffffffff0f", "80808080808080808001", "ffffffffffffffffff01"}
var svKnown = []int64{0, -1, 1, -2, 2, 63, -64, 64, -9223372036854775808, 9223372036854775807}
var svEnc = []string{"00", "01", "02", "03", "04", "7e", "7f", "8001", "ffffffffffffffffff01", "feffffffffffffffff01"}

func TestAppendUvarint_known_answers(t *testing.T) {
	for i, v := range uvKnown {
		if got := hex.EncodeToString(AppendUvarint(nil, v)); got != uvEnc[i] {
			t.Errorf("AppendUvarint(%d) = %s; want %s", v, got, uvEnc[i])
		}
	}
	got := AppendUvarint([]byte{9, 9}, 300)
	if !bytes.Equal(got, []byte{9, 9, 0xAC, 0x02}) {
		t.Errorf("AppendUvarint must append to dst: got % x", got)
	}
}

func TestUvarint_decodes_known_answers_and_reports_length(t *testing.T) {
	for i, v := range uvKnown {
		enc := append(unhex(uvEnc[i]), 0xAA, 0xBB) // trailing bytes must be left alone
		got, n, err := Uvarint(enc)
		if err != nil || got != v || n != len(enc)-2 {
			t.Errorf("Uvarint(%s ..) = %d, n=%d, %v; want %d, n=%d", uvEnc[i], got, n, err, v, len(enc)-2)
		}
	}
}

func TestUvarint_round_trip_random(t *testing.T) {
	r := &tRand{s: 3}
	for i := 0; i < 5000; i++ {
		v := (r.next()<<31 ^ r.next()) >> uint(r.intn(64))
		enc := AppendUvarint(nil, v)
		got, n, err := Uvarint(enc)
		if err != nil || got != v || n != len(enc) {
			t.Fatalf("%d: got %d, n=%d, err=%v (encoding % x)", v, got, n, err, enc)
		}
		for cut := 0; cut < len(enc); cut++ {
			if _, n, err := Uvarint(enc[:cut]); !errors.Is(err, ErrShort) || n != 0 {
				t.Fatalf("%d cut to %d bytes: n=%d err=%v; want 0, ErrShort", v, cut, n, err)
			}
		}
	}
}

func TestUvarint_rejects_bad_encodings(t *testing.T) {
	nine := bytes.Repeat([]byte{0xFF}, 9)
	cases := []struct {
		name string
		in   []byte
		want error
	}{
		{"empty", nil, ErrShort},
		{"continuation then nothing", []byte{0x80}, ErrShort},
		{"nine bytes, still going", bytes.Repeat([]byte{0x80}, 9), ErrShort},
		{"tenth byte too big", append(append([]byte{}, nine...), 0x02), ErrOverflow},
		{"tenth byte keeps going", append(append([]byte{}, nine...), 0x80, 0x01), ErrOverflow},
		{"eleven continuation bytes", bytes.Repeat([]byte{0xFF}, 11), ErrOverflow},
		{"zero padded", []byte{0x80, 0x00}, ErrNonCanonical},
		{"zero padded longer", []byte{0xFF, 0x80, 0x00}, ErrNonCanonical},
		{"ten bytes ending in zero", append(bytes.Repeat([]byte{0x80}, 9), 0x00), ErrNonCanonical},
	}
	for _, c := range cases {
		if v, n, err := Uvarint(c.in); !errors.Is(err, c.want) || v != 0 || n != 0 {
			t.Errorf("%s: Uvarint(% x) = %d, %d, %v; want 0, 0, %v", c.name, c.in, v, n, err, c.want)
		}
	}
	if v, n, err := Uvarint(append(append([]byte{}, nine...), 0x01)); err != nil || v != math.MaxUint64 || n != 10 {
		t.Errorf("largest value: %d, %d, %v", v, n, err)
	}
}

func TestVarint_zigzag(t *testing.T) {
	for i, v := range svKnown {
		if got := hex.EncodeToString(AppendVarint(nil, v)); got != svEnc[i] {
			t.Errorf("AppendVarint(%d) = %s; want %s", v, got, svEnc[i])
		}
		got, n, err := Varint(unhex(svEnc[i]))
		if err != nil || got != v || n != len(svEnc[i])/2 {
			t.Errorf("Varint(%s) = %d, %d, %v; want %d", svEnc[i], got, n, err, v)
		}
	}
	r := &tRand{s: 11}
	for i := 0; i < 3000; i++ {
		v := int64(r.next()<<31^r.next()) >> uint(r.intn(64))
		got, _, err := Varint(AppendVarint(nil, v))
		if err != nil || got != v {
			t.Fatalf("round trip of %d gave %d, %v", v, got, err)
		}
	}
	if _, _, err := Varint([]byte{0x80, 0x00}); !errors.Is(err, ErrNonCanonical) {
		t.Errorf("Varint must pass decoding errors through: %v", err)
	}
}
