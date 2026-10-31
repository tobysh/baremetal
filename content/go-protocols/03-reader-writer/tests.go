package main

import (
	"bytes"
	"errors"
	"hash/crc32"
	"io"
	"math/rand"
	"testing"
)

func refFrame(typ byte, p []byte) []byte {
	b := []byte{0xBE, typ, byte(len(p) >> 8), byte(len(p))}
	b = append(b, p...)
	s := crc32.ChecksumIEEE(b)
	return append(b, byte(s>>24), byte(s>>16), byte(s>>8), byte(s))
}

// dribble returns at most max(1, pick()) bytes per Read, sometimes (0, nil),
// and can deliver the final bytes together with io.EOF.
type dribble struct {
	data     []byte
	pos      int
	r        *rand.Rand
	maxChunk int
	eofWith  bool
	zeros    bool
	failAt   int // return failErr when pos reaches this (if >0)
	failErr  error
}

func (d *dribble) Read(p []byte) (int, error) {
	if d.failAt > 0 && d.pos >= d.failAt {
		return 0, d.failErr
	}
	if d.pos >= len(d.data) {
		return 0, io.EOF
	}
	if d.zeros && d.r.Intn(4) == 0 {
		return 0, nil
	}
	n := 1 + d.r.Intn(d.maxChunk)
	if n > len(p) {
		n = len(p)
	}
	if n > len(d.data)-d.pos {
		n = len(d.data) - d.pos
	}
	if d.failAt > 0 && d.pos+n > d.failAt {
		n = d.failAt - d.pos
	}
	copy(p, d.data[d.pos:d.pos+n])
	d.pos += n
	if d.eofWith && d.pos == len(d.data) {
		return n, io.EOF
	}
	return n, nil
}

func newDribble(data []byte, seed int64, max int) *dribble {
	return &dribble{data: data, r: rand.New(rand.NewSource(seed)), maxChunk: max}
}

func TestReadFrame_sequence_with_odd_readers(t *testing.T) {
	for seed := int64(0); seed < 80; seed++ {
		r := rand.New(rand.NewSource(seed))
		var stream []byte
		var want []Frame
		for i := 0; i < 1+int(seed%6); i++ {
			p := make([]byte, r.Intn(600))
			r.Read(p)
			typ := byte(r.Intn(200))
			want = append(want, Frame{typ, p})
			stream = append(stream, refFrame(typ, p)...)
		}
		d := newDribble(stream, seed, []int{1, 3, 17, 2000}[seed%4])
		d.eofWith = seed%2 == 0
		d.zeros = seed%3 == 0
		for i, w := range want {
			f, err := ReadFrame(d)
			if err != nil || f.Type != w.Type || !bytes.Equal(f.Payload, w.Payload) {
				t.Fatalf("seed %d frame %d: type %d len %d err %v; want type %d len %d", seed, i, f.Type, len(f.Payload), err, w.Type, len(w.Payload))
			}
		}
		if _, err := ReadFrame(d); err != io.EOF {
			t.Fatalf("seed %d: after the last frame got %v; want io.EOF", seed, err)
		}
	}
}

func TestReadFrame_does_not_over_read(t *testing.T) {
	a := refFrame(1, []byte("first"))
	b := refFrame(2, []byte("second"))
	rd := bytes.NewReader(append(append([]byte(nil), a...), b...))
	if _, err := ReadFrame(rd); err != nil {
		t.Fatal(err)
	}
	if consumed := len(a) + len(b) - rd.Len(); consumed != len(a) {
		t.Errorf("consumed %d bytes for a %d byte frame", consumed, len(a))
	}
}

func TestReadFrame_cut_at_every_point(t *testing.T) {
	f := refFrame(4, []byte("some payload bytes"))
	for cut := 0; cut < len(f); cut++ {
		_, err := ReadFrame(bytes.NewReader(f[:cut]))
		want := io.ErrUnexpectedEOF
		if cut == 0 {
			want = io.EOF
		}
		if err != want {
			t.Fatalf("cut at %d of %d: %v; want %v", cut, len(f), err, want)
		}
	}
	// the same with a reader that attaches EOF to its final bytes
	for cut := 1; cut < len(f); cut++ {
		d := newDribble(f[:cut], int64(cut), 5)
		d.eofWith = true
		if _, err := ReadFrame(d); err != io.ErrUnexpectedEOF {
			t.Fatalf("(n, EOF) reader cut at %d: %v; want ErrUnexpectedEOF", cut, err)
		}
	}
}

func TestReadFrame_damage(t *testing.T) {
	f := refFrame(1, []byte("payload"))
	bad := append([]byte(nil), f...)
	bad[6] ^= 1
	if _, err := ReadFrame(bytes.NewReader(bad)); !errors.Is(err, ErrChecksum) {
		t.Errorf("flipped payload bit: %v; want ErrChecksum", err)
	}
	if _, err := ReadFrame(bytes.NewReader([]byte("GET / HTTP/1.1\r\n"))); !errors.Is(err, ErrMagic) {
		t.Errorf("text on the wire: %v; want ErrMagic", err)
	}
}

func TestReadFrame_oversize_header_reads_only_header(t *testing.T) {
	data := append([]byte{0xBE, 1, 0xFF, 0xFF}, make([]byte, 100000)...)
	rd := bytes.NewReader(data)
	_, err := ReadFrame(rd)
	if !errors.Is(err, ErrTooLarge) {
		t.Fatalf("got %v; want ErrTooLarge", err)
	}
	if used := len(data) - rd.Len(); used > HeaderLen {
		t.Errorf("read %d bytes before rejecting; want at most the %d header bytes", used, HeaderLen)
	}
}

func TestReadFrame_reader_error_passes_through(t *testing.T) {
	boom := errors.New("link reset")
	f := refFrame(1, []byte("0123456789"))
	for _, at := range []int{2, 5, 9} {
		d := newDribble(f, 1, 3)
		d.failAt, d.failErr = at, boom
		if _, err := ReadFrame(d); !errors.Is(err, boom) {
			t.Errorf("error after %d bytes: %v; want the reader's error", at, err)
		}
	}
}

type recWriter struct {
	calls  [][]byte
	limit  int // accept only this many bytes per call if >0
	err    error
	silent bool // short write but nil error (contract violation)
}

func (w *recWriter) Write(p []byte) (int, error) {
	w.calls = append(w.calls, append([]byte(nil), p...))
	n := len(p)
	if w.limit > 0 && n > w.limit {
		n = w.limit
		if !w.silent {
			return n, io.ErrShortWrite
		}
	}
	if w.err != nil {
		return 0, w.err
	}
	return n, nil
}

func TestWriteFrame_single_write_of_the_whole_frame(t *testing.T) {
	for _, n := range []int{0, 1, 77, 1024} {
		w := &recWriter{}
		p := bytes.Repeat([]byte{byte(n)}, n)
		if err := WriteFrame(w, 9, p); err != nil {
			t.Fatal(err)
		}
		if len(w.calls) != 1 || !bytes.Equal(w.calls[0], refFrame(9, p)) {
			t.Errorf("payload %d: %d Write calls; want exactly 1 with the whole frame", n, len(w.calls))
		}
	}
}

func TestWriteFrame_errors(t *testing.T) {
	w := &recWriter{}
	if err := WriteFrame(w, 1, make([]byte, MaxPayload+1)); !errors.Is(err, ErrTooLarge) || len(w.calls) != 0 {
		t.Errorf("oversize: %v with %d writes; want ErrTooLarge and none", err, len(w.calls))
	}
	boom := errors.New("pipe closed")
	if err := WriteFrame(&recWriter{err: boom}, 1, []byte("x")); !errors.Is(err, boom) {
		t.Errorf("writer error: %v; want it returned", err)
	}
	if err := WriteFrame(&recWriter{limit: 3, silent: true}, 1, []byte("hello")); err != io.ErrShortWrite {
		t.Errorf("silent short write: %v; want io.ErrShortWrite", err)
	}
	if err := WriteFrame(&recWriter{limit: 3}, 1, []byte("hello")); err == nil {
		t.Errorf("short write with error was swallowed")
	}
}

func TestWriteThenRead_round_trip(t *testing.T) {
	var buf bytes.Buffer
	for i := 0; i < 30; i++ {
		if err := WriteFrame(&buf, byte(i), bytes.Repeat([]byte{byte(i)}, i*7)); err != nil {
			t.Fatal(err)
		}
	}
	d := newDribble(buf.Bytes(), 5, 9)
	for i := 0; i < 30; i++ {
		f, err := ReadFrame(d)
		if err != nil || f.Type != byte(i) || len(f.Payload) != i*7 {
			t.Fatalf("frame %d: %v %d %d", i, err, f.Type, len(f.Payload))
		}
	}
}
