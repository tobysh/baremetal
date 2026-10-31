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

type packCase struct{ in, enc string }

var packKnown = []packCase{{"", "00"}, {"78", "0078"}, {"616263", "00616263"}, {"61616161", "018261"}, {"61626363636363636363636363636363636363636363636363636363636363636363636363636363636363636363636363636363", "01016162b063"}, {"616263616263", "00616263616263"}, {"000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f2021222324252627", "00000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f2021222324252627"}}

// refLimited applies the rules of the task to a token stream, token by token.
func refLimited(enc []byte, lim Limits) ([]byte, error) {
	var out []byte
	i := 0
	for i < len(enc) {
		c := int(enc[i])
		n, need := c+1, c+2
		if c >= 128 {
			n, need = c-126, 2
		}
		if i+need > len(enc) {
			return nil, ErrTruncated
		}
		total, consumed := len(out)+n, i+need
		if total > lim.MaxOutput {
			return nil, ErrTooLarge
		}
		if lim.MaxRatio > 0 && total > lim.RatioFloor && total > lim.MaxRatio*consumed {
			return nil, ErrTooLarge
		}
		if c < 128 {
			out = append(out, enc[i+1:i+need]...)
		} else {
			out = append(out, bytes.Repeat(enc[i+1:i+2], n)...)
		}
		i += need
	}
	return out, nil
}

func TestRLEDecodeLimited_boundaries(t *testing.T) {
	enc := []byte{0x02, 'a', 'b', 'c', 0xFF, 'z'} // 3 literals + 129 z = 132 bytes
	if got, err := RLEDecodeLimited(enc, Limits{MaxOutput: 132}); err != nil || len(got) != 132 {
		t.Errorf("output exactly at the limit must pass: %d bytes, %v", len(got), err)
	}
	if _, err := RLEDecodeLimited(enc, Limits{MaxOutput: 131}); !errors.Is(err, ErrTooLarge) {
		t.Errorf("one byte over the limit: err = %v; want ErrTooLarge", err)
	}
	if got, err := RLEDecodeLimited(nil, Limits{}); err != nil || len(got) != 0 {
		t.Errorf("empty input with MaxOutput 0: %v, %v", got, err)
	}
	if _, err := RLEDecodeLimited([]byte{0x00, 'x'}, Limits{MaxOutput: 0}); !errors.Is(err, ErrTooLarge) {
		t.Errorf("MaxOutput 0 permits no output: %v", err)
	}
	if _, err := RLEDecodeLimited([]byte{0x05, 'a'}, Limits{MaxOutput: 1}); !errors.Is(err, ErrTruncated) {
		t.Errorf("a truncated token is reported as truncated even when it would also exceed the limit: %v", err)
	}
}

func TestRLEDecodeLimited_ratio_with_floor(t *testing.T) {
	lim := Limits{MaxOutput: 1 << 30, MaxRatio: 16, RatioFloor: 1000}
	small := []byte{0xFF, 'x', 0xFF, 'x', 0xFF, 'x'} // 387 bytes from 6: ratio 64 but under the floor
	if got, err := RLEDecodeLimited(small, lim); err != nil || len(got) != 387 {
		t.Errorf("a small, highly repetitive input below the floor must pass: %d, %v", len(got), err)
	}
	var big []byte
	for i := 0; i < 20; i++ {
		big = append(big, 0xFF, 'x') // 20*129 = 2580 bytes from 40
	}
	if _, err := RLEDecodeLimited(big, lim); !errors.Is(err, ErrTooLarge) {
		t.Errorf("2580 bytes from 40 (ratio 64) above the floor: err = %v; want ErrTooLarge", err)
	}
	lit := append([]byte{127}, bytes.Repeat([]byte{'q'}, 128)...) // ratio below 1
	if _, err := RLEDecodeLimited(lit, lim); err != nil {
		t.Errorf("literals never trip the ratio: %v", err)
	}
}

func TestRLEDecodeLimited_matches_reference(t *testing.T) {
	r := &tRand{s: 17}
	for i := 0; i < 2000; i++ {
		var enc []byte
		for k := r.intn(12); k > 0; k-- {
			if r.intn(2) == 0 {
				n := 1 + r.intn(20)
				enc = append(enc, byte(n-1))
				for j := 0; j < n; j++ {
					enc = append(enc, byte(r.intn(256)))
				}
			} else {
				enc = append(enc, byte(128+r.intn(128)), byte(r.intn(256)))
			}
		}
		if r.intn(5) == 0 && len(enc) > 0 {
			enc = enc[:r.intn(len(enc))]
		}
		lim := Limits{MaxOutput: r.intn(900), MaxRatio: r.intn(40), RatioFloor: r.intn(300)}
		got, err := RLEDecodeLimited(enc, lim)
		want, werr := refLimited(enc, lim)
		if (err == nil) != (werr == nil) || (err != nil && !errors.Is(err, werr)) || !bytes.Equal(got, want) {
			t.Fatalf("enc % x limits %+v:\n got %d bytes, %v\nwant %d bytes, %v", enc, lim, len(got), err, len(want), werr)
		}
	}
}

func TestUnpack_stops_a_bomb_before_allocating_it(t *testing.T) {
	bomb := []byte{1}
	for i := 0; i < 1_000_000; i++ { // 2 MB of input would expand to 129 MB
		bomb = append(bomb, 0xFF, 'A')
	}
	var err error
	a := allocated(func() { _, err = Unpack(bomb, Limits{MaxOutput: 1 << 20}) })
	if !errors.Is(err, ErrTooLarge) {
		t.Fatalf("bomb: err = %v; want ErrTooLarge", err)
	}
	if a > 8<<20 {
		t.Errorf("stopping the bomb allocated %d bytes; a 1 MiB limit should need about 2 MiB, not the whole expansion", a)
	}
}

func TestPack_known_answers_and_bound(t *testing.T) {
	for _, c := range packKnown {
		if got := hex.EncodeToString(Pack(unhex(c.in))); got != c.enc {
			t.Errorf("Pack(%q) = %s; want %s", unhex(c.in), got, c.enc)
		}
	}
	// Every string up to length 5 over {a,b,c} never grows by more than the mode byte.
	var walk func(cur []byte)
	walk = func(cur []byte) {
		p := Pack(cur)
		if len(p) > len(cur)+1 {
			t.Fatalf("Pack(%q) is %d bytes; at most %d allowed", cur, len(p), len(cur)+1)
		}
		got, err := Unpack(p, Limits{MaxOutput: 100})
		if err != nil || !bytes.Equal(got, cur) {
			t.Fatalf("Unpack(Pack(%q)) = %q, %v", cur, got, err)
		}
		if len(cur) < 5 {
			for _, b := range "abc" {
				walk(append(append([]byte{}, cur...), byte(b)))
			}
		}
	}
	walk(nil)
	r := &tRand{s: 6}
	noise := make([]byte, 20000)
	for i := range noise {
		noise[i] = byte(r.next())
	}
	if p := Pack(noise); len(p) != len(noise)+1 || p[0] != 0 {
		t.Errorf("incompressible data: %d bytes, mode %d; want stored (mode 0) with exactly 1 byte of overhead", len(p), p[0])
	}
	if p := Pack(bytes.Repeat([]byte{'z'}, 5000)); p[0] != 1 || len(p) > 1+2*39 {
		t.Errorf("5000 equal bytes should use RLE: mode %d, %d bytes", p[0], len(p))
	}
}

func TestUnpack_errors_and_limits(t *testing.T) {
	lim := Limits{MaxOutput: 4}
	cases := []struct {
		name string
		in   []byte
		want error
	}{
		{"empty", nil, ErrTruncated},
		{"unknown mode", []byte{7, 1, 2}, ErrCorrupt},
		{"stored over the limit", []byte{0, 1, 2, 3, 4, 5}, ErrTooLarge},
		{"rle truncated", []byte{1, 0x03, 'a'}, ErrTruncated},
		{"rle over the limit", []byte{1, 0x84, 'a'}, ErrTooLarge},
	}
	for _, c := range cases {
		if _, err := Unpack(c.in, lim); !errors.Is(err, c.want) {
			t.Errorf("%s: err = %v; want %v", c.name, err, c.want)
		}
	}
	if got, err := Unpack([]byte{0, 1, 2, 3, 4}, lim); err != nil || len(got) != 4 {
		t.Errorf("stored exactly at the limit: %v, %v", got, err)
	}
}
