package main

import (
	"bytes"
	"encoding/hex"
	"errors"
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

type rleCase struct{ in, enc string }

var rleKnown = []rleCase{{"", ""}, {"616161626363", "816102626363"}, {"61616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161", "ff610061"}, {"61616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616162", "ff610062"}, {"787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878787878", "ff78ff78"}, {"6162616261626162", "076162616261626162"}, {"6161626261616262", "076161626261616262"}, {"000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f8081", "7f000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f018081"}, {"61626262626262637a7a7a71", "006184620063817a0071"}}

type refTok struct {
	rep  bool
	b    byte
	n    int
	lits []byte
}

// refRLE builds runs first, then tokens: a different route to the same format.
func refRLE(d []byte) []byte {
	var toks []refTok
	var lit []byte
	flush := func() {
		for len(lit) > 0 {
			k := len(lit)
			if k > 128 {
				k = 128
			}
			toks = append(toks, refTok{lits: append([]byte{}, lit[:k]...)})
			lit = lit[k:]
		}
	}
	for i := 0; i < len(d); {
		j := i
		for j < len(d) && d[j] == d[i] {
			j++
		}
		run := j - i
		for run >= 3 {
			k := run
			if k > 129 {
				k = 129
			}
			flush()
			toks = append(toks, refTok{rep: true, b: d[i], n: k})
			run -= k
		}
		for ; run > 0; run-- {
			lit = append(lit, d[i])
		}
		i = j
	}
	flush()
	var out []byte
	for _, tk := range toks {
		if tk.rep {
			out = append(out, byte(128+tk.n-2), tk.b)
		} else {
			out = append(out, byte(len(tk.lits)-1))
			out = append(out, tk.lits...)
		}
	}
	return out
}

func rleInputs() [][]byte {
	r := &tRand{s: 5}
	var ins [][]byte
	for i := 0; i < 200; i++ {
		var d []byte
		for len(d) < r.intn(700) {
			b := byte('a' + r.intn(3))
			n := 1 + r.intn(4)
			if r.intn(6) == 0 {
				n = 120 + r.intn(300)
			}
			d = append(d, bytes.Repeat([]byte{b}, n)...)
		}
		ins = append(ins, d)
	}
	return ins
}

func TestEncode_known_answers(t *testing.T) {
	for _, c := range rleKnown {
		in, want := unhex(c.in), c.enc
		if got := hex.EncodeToString(RLEEncode(in)); got != want {
			t.Errorf("RLEEncode(%q) = %s; want %s", in, got, want)
		}
	}
}

func TestEncode_matches_reference_on_many_inputs(t *testing.T) {
	for i, d := range rleInputs() {
		got, want := RLEEncode(d), refRLE(d)
		if !bytes.Equal(got, want) {
			t.Fatalf("input %d (%d bytes): encoding differs from the reference\n got % x\nwant % x", i, len(d), got, want)
		}
		if limit := len(d) + (len(d)+127)/128; len(got) > limit {
			t.Fatalf("input %d: %d encoded bytes for %d input bytes; the worst case is %d", i, len(got), len(d), limit)
		}
	}
}

func TestDecode_known_answers_and_noncanonical_input(t *testing.T) {
	for _, c := range rleKnown {
		got, err := RLEDecode(unhex(c.enc))
		if err != nil || !bytes.Equal(got, unhex(c.in)) {
			t.Errorf("RLEDecode(%s) = %q, %v; want %q", c.enc, got, err, unhex(c.in))
		}
	}
	// A repeat of only 2, and two literal tokens in a row, are legal even though our encoder never writes them.
	if got, err := RLEDecode([]byte{0x80, 'x', 0x00, 'y', 0x00, 'z'}); err != nil || string(got) != "xxyz" {
		t.Errorf("non-canonical but valid stream: %q, %v; want \"xxyz\"", got, err)
	}
}

func TestDecode_round_trip_and_truncation(t *testing.T) {
	for i, d := range rleInputs() {
		enc := RLEEncode(d)
		if got, err := RLEDecode(enc); err != nil || !bytes.Equal(got, d) {
			t.Fatalf("input %d: round trip failed (err=%v)", i, err)
		}
		if i%10 != 0 {
			continue
		}
		for cut := 0; cut < len(enc); cut++ {
			got, err := RLEDecode(enc[:cut])
			if err == nil && !bytes.HasPrefix(d, got) {
				t.Fatalf("input %d cut at %d: decoded %d bytes that are not a prefix of the original", i, cut, len(got))
			}
			if err != nil && !errors.Is(err, ErrTruncated) {
				t.Fatalf("input %d cut at %d: err = %v; want ErrTruncated or success at a token boundary", i, cut, err)
			}
		}
	}
	for _, bad := range [][]byte{{0x05, 'a', 'b'}, {0x80}, {0xFF}, {0x00}} {
		if _, err := RLEDecode(bad); !errors.Is(err, ErrTruncated) {
			t.Errorf("RLEDecode(% x): err = %v; want ErrTruncated", bad, err)
		}
	}
}
