package main

import (
	"bytes"
	"errors"
	"fmt"
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

// cleanFrame returns a frame whose bytes after the first contain no Magic byte.
func cleanFrame(r *rand.Rand, typ byte) ([]byte, []byte) {
	for {
		p := make([]byte, r.Intn(90))
		for i := range p {
			p[i] = byte(r.Intn(255))
			if p[i] == 0xBE {
				p[i] = 0xBD
			}
		}
		f := refFrame(typ, p)
		if bytes.IndexByte(f[1:], 0xBE) < 0 {
			return f, p
		}
	}
}

func noise(r *rand.Rand, n int) []byte { // no Magic bytes
	b := make([]byte, n)
	for i := range b {
		b[i] = byte(r.Intn(255))
		if b[i] == 0xBE {
			b[i] = 0
		}
	}
	return b
}

type result struct {
	frames []Frame
	errs   []error
	s      *Scanner
	fed    int
}

func run(t *testing.T, stream []byte, next func() int) result {
	t.Helper()
	defer func() {
		if e := recover(); e != nil {
			t.Fatalf("panic on input of %d bytes: %v", len(stream), e)
		}
	}()
	res := result{s: NewScanner(), fed: len(stream)}
	for pos := 0; pos < len(stream); {
		n := next()
		if n > len(stream)-pos {
			n = len(stream) - pos
		}
		f, e := res.s.Feed(append([]byte(nil), stream[pos:pos+n]...))
		res.frames, res.errs = append(res.frames, f...), append(res.errs, e...)
		pos += n
		if res.s.Buffered() >= HeaderLen+MaxPayload+TrailerLen {
			t.Fatalf("Buffered = %d: more than a frame is being held", res.s.Buffered())
		}
	}
	f, e := res.s.Close()
	res.frames, res.errs = append(res.frames, f...), append(res.errs, e...)
	if res.s.Buffered() != 0 {
		t.Fatalf("Buffered = %d after Close", res.s.Buffered())
	}
	return res
}

func accounted(t *testing.T, r result) {
	t.Helper()
	sum := 0
	for _, f := range r.frames {
		sum += HeaderLen + len(f.Payload) + TrailerLen
	}
	if sum+r.s.Dropped() != r.fed {
		t.Fatalf("bytes fed %d != frames %d + dropped %d", r.fed, sum, r.s.Dropped())
	}
}

func same(a, b []Frame) bool {
	if len(a) != len(b) {
		return false
	}
	for i := range a {
		if a[i].Type != b[i].Type || !bytes.Equal(a[i].Payload, b[i].Payload) {
			return false
		}
	}
	return true
}

func TestClean_stream_has_no_errors(t *testing.T) {
	r := rand.New(rand.NewSource(1))
	var stream []byte
	var want []Frame
	for i := 0; i < 20; i++ {
		f, p := cleanFrame(r, byte(i))
		stream = append(stream, f...)
		want = append(want, Frame{byte(i), p})
	}
	res := run(t, stream, func() int { return 1 + r.Intn(40) })
	if !same(res.frames, want) || len(res.errs) != 0 || res.s.Dropped() != 0 {
		t.Fatalf("got %d frames, %d errors, %d dropped; want %d, 0, 0", len(res.frames), len(res.errs), res.s.Dropped(), len(want))
	}
}

func TestGarbage_between_frames_is_skipped(t *testing.T) {
	for seed := int64(0); seed < 60; seed++ {
		r := rand.New(rand.NewSource(seed))
		var stream []byte
		var want []Frame
		regions := 0
		for i := 0; i < 8; i++ {
			if r.Intn(2) == 0 {
				stream = append(stream, noise(r, 1+r.Intn(30))...)
				regions++
			}
			f, p := cleanFrame(r, byte(i))
			stream = append(stream, f...)
			want = append(want, Frame{byte(i), p})
		}
		res := run(t, stream, func() int { return 1 + r.Intn(25) })
		if !same(res.frames, want) {
			t.Fatalf("seed %d: recovered %d frames; want %d", seed, len(res.frames), len(want))
		}
		if len(res.errs) != regions {
			t.Fatalf("seed %d: %d errors; want one per garbage region (%d)", seed, len(res.errs), regions)
		}
		for _, e := range res.errs {
			if !errors.Is(e, ErrMagic) {
				t.Fatalf("seed %d: error %v; want ErrMagic for noise", seed, e)
			}
		}
		accounted(t, res)
	}
}

func TestCorrupted_frame_is_dropped_neighbours_survive(t *testing.T) {
	for seed := int64(0); seed < 60; seed++ {
		r := rand.New(rand.NewSource(seed))
		a, pa := cleanFrame(r, 1)
		bad, _ := cleanFrame(r, 2)
		for len(bad) < 12 {
			bad, _ = cleanFrame(r, 2)
		}
		bad[4+r.Intn(len(bad)-8)] ^= 0x21 // payload byte (cannot become Magic: 0xBE^0x21 is checked)
		if bytes.IndexByte(bad[1:], 0xBE) >= 0 {
			continue
		}
		c, pc := cleanFrame(r, 3)
		stream := append(append(append([]byte(nil), a...), bad...), c...)
		res := run(t, stream, func() int { return 1 + r.Intn(30) })
		want := []Frame{{1, pa}, {3, pc}}
		if !same(res.frames, want) {
			t.Fatalf("seed %d: frames %d; want the two intact neighbours", seed, len(res.frames))
		}
		if len(res.errs) != 1 || !errors.Is(res.errs[0], ErrChecksum) {
			t.Fatalf("seed %d: errors %v; want one ErrChecksum", seed, res.errs)
		}
		accounted(t, res)
	}
}

func TestFalse_header_does_not_swallow_real_frames(t *testing.T) {
	r := rand.New(rand.NewSource(9))
	f1, p1 := cleanFrame(r, 1)
	f2, p2 := cleanFrame(r, 2)
	// A fake header promising 900 payload bytes, followed by two real frames.
	stream := append([]byte{0xBE, 0x09, 0x03, 0x84}, append(f1, f2...)...)
	stream = append(stream, noise(r, 900)...)
	res := run(t, stream, func() int { return 7 })
	if !same(res.frames, []Frame{{1, p1}, {2, p2}}) {
		t.Fatalf("recovered %d frames; the scanner must retry one byte after a failed attempt", len(res.frames))
	}
	// Same stream, but it ends before the fake frame could complete: Close must recover.
	short := append([]byte{0xBE, 0x09, 0x03, 0x84}, append(append([]byte(nil), f1...), f2...)...)
	res = run(t, short, func() int { return 5 })
	if !same(res.frames, []Frame{{1, p1}, {2, p2}}) {
		t.Fatalf("with a truncated fake frame at the end: recovered %d frames, errs %v", len(res.frames), res.errs)
	}
	if len(res.errs) != 1 || !errors.Is(res.errs[0], ErrTruncated) {
		t.Fatalf("errs %v; want one ErrTruncated", res.errs)
	}
}

func TestOversize_header_reported_immediately(t *testing.T) {
	s := NewScanner()
	frames, errs := s.Feed([]byte{0xBE, 1, 0xFF, 0xFF})
	if len(frames) != 0 || len(errs) != 1 || !errors.Is(errs[0], ErrTooLarge) {
		t.Fatalf("got %d frames, errs %v; want ErrTooLarge now, without waiting for 65535 bytes", len(frames), errs)
	}
	if s.Buffered() >= 4 {
		t.Errorf("Buffered = %d after rejecting the header", s.Buffered())
	}
}

func TestTruncated_tail(t *testing.T) {
	r := rand.New(rand.NewSource(4))
	f, p := cleanFrame(r, 5)
	g, _ := cleanFrame(r, 6)
	res := run(t, append(append([]byte(nil), f...), g[:len(g)-2]...), func() int { return 3 })
	if !same(res.frames, []Frame{{5, p}}) || len(res.errs) != 1 || !errors.Is(res.errs[0], ErrTruncated) {
		t.Fatalf("frames %d errs %v; want the good frame and ErrTruncated", len(res.frames), res.errs)
	}
	accounted(t, res)
}

func mutate(r *rand.Rand, b []byte) []byte {
	b = append([]byte(nil), b...)
	for k := 0; k < 1+r.Intn(6); k++ {
		if len(b) == 0 {
			break
		}
		i := r.Intn(len(b))
		switch r.Intn(5) {
		case 0:
			b[i] ^= byte(1 << r.Intn(8))
		case 1:
			b[i] = byte(r.Intn(256))
		case 2:
			b = append(b[:i], b[i+1:]...)
		case 3:
			b = append(b[:i], append(noise(r, 1+r.Intn(5)), b[i:]...)...)
		case 4:
			if r.Intn(3) == 0 {
				b = b[:i]
			}
		}
	}
	return b
}

func TestMutated_streams_invariants_and_chunk_independence(t *testing.T) {
	for seed := int64(0); seed < 300; seed++ {
		r := rand.New(rand.NewSource(seed))
		var stream []byte
		for i := 0; i < 1+r.Intn(8); i++ {
			p := make([]byte, r.Intn(300))
			r.Read(p)
			stream = append(stream, refFrame(byte(i), p)...)
		}
		stream = mutate(r, stream)
		whole := run(t, stream, func() int { return len(stream) + 1 })
		accounted(t, whole)
		for _, f := range whole.frames {
			if !bytes.Contains(stream, refFrame(f.Type, f.Payload)) {
				t.Fatalf("seed %d: returned a frame (type %d, %d bytes) that is not in the input", seed, f.Type, len(f.Payload))
			}
		}
		for name, next := range map[string]func() int{
			"bytewise": func() int { return 1 },
			"random":   func() int { return r.Intn(70) },
		} {
			got := run(t, stream, next)
			if !same(got.frames, whole.frames) || len(got.errs) != len(whole.errs) || got.s.Dropped() != whole.s.Dropped() {
				t.Fatalf("seed %d, %s chunks: %d frames %d errors %d dropped; whole stream gave %d/%d/%d", seed, name,
					len(got.frames), len(got.errs), got.s.Dropped(), len(whole.frames), len(whole.errs), whole.s.Dropped())
			}
		}
	}
}

func TestRandom_bytes_never_panic_and_stay_bounded(t *testing.T) {
	for seed := int64(0); seed < 200; seed++ {
		r := rand.New(rand.NewSource(1000 + seed))
		b := make([]byte, r.Intn(6000))
		for i := range b {
			if r.Intn(8) == 0 {
				b[i] = 0xBE
			} else {
				b[i] = byte(r.Intn(256))
			}
		}
		res := run(t, b, func() int { return 1 + r.Intn(500) })
		accounted(t, res)
		for _, e := range res.errs {
			if !errors.Is(e, ErrMagic) && !errors.Is(e, ErrTooLarge) && !errors.Is(e, ErrChecksum) && !errors.Is(e, ErrTruncated) {
				t.Fatalf("seed %d: unexpected error %v", seed, fmt.Sprint(e))
			}
		}
	}
}
