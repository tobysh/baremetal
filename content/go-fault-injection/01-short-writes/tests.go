package main

import (
	"bytes"
	"errors"
	"io"
	"math/rand"
	"testing"
	"testing/iotest"
)

var errBoom = errors.New("boom")

type scripted struct {
	sink   bytes.Buffer
	chunks []int
	failAt int
	calls  int
}

func (s *scripted) Write(p []byte) (int, error) {
	i := s.calls
	s.calls++
	limit := s.chunks[len(s.chunks)-1]
	if i < len(s.chunks) {
		limit = s.chunks[i]
	}
	n := len(p)
	if limit < n {
		n = limit
	}
	var err error
	if s.failAt >= 0 && s.sink.Len()+n >= s.failAt {
		n = s.failAt - s.sink.Len()
		err = errBoom
	}
	s.sink.Write(p[:n])
	return n, err
}

func payload(seed int64, n int) []byte {
	b := make([]byte, n)
	rand.New(rand.NewSource(seed)).Read(b)
	return b
}

var schedules = [][]int{{1}, {3, 0, 2, 100}, {7}, {0, 0, 5, 1}, {2, 2, 2, 1000}, {100000}}

func TestWriteFull_all_bytes_under_every_schedule(t *testing.T) {
	for _, size := range []int{1, 2, 9, 64, 300} {
		p := payload(int64(size), size)
		for _, sc := range schedules {
			w := &scripted{chunks: sc, failAt: -1}
			n, err := WriteFull(w, p)
			if err != nil || n != len(p) {
				t.Fatalf("size %d chunks %v: n=%d err=%v, want %d and nil", size, sc, n, err, len(p))
			}
			if !bytes.Equal(w.sink.Bytes(), p) {
				t.Fatalf("size %d chunks %v: sink differs from payload (bytes lost or duplicated)", size, sc)
			}
		}
	}
}

func TestWriteFull_error_at_every_byte_position(t *testing.T) {
	p := payload(5, 40)
	for _, sc := range schedules {
		for k := 0; k < len(p); k++ {
			w := &scripted{chunks: sc, failAt: k}
			n, err := WriteFull(w, p)
			if !errors.Is(err, errBoom) {
				t.Fatalf("chunks %v failAt %d: err = %v, want the writer's error", sc, k, err)
			}
			if n != k || !bytes.Equal(w.sink.Bytes(), p[:k]) {
				t.Fatalf("chunks %v failAt %d: returned n=%d, sink has %d bytes; want exactly the %d accepted bytes as a prefix", sc, k, n, w.sink.Len(), k)
			}
		}
	}
}

func TestWriteFull_gives_up_on_a_stalled_writer(t *testing.T) {
	w := &scripted{chunks: []int{0}, failAt: -1}
	n, err := WriteFull(w, []byte("never accepted"))
	if err != io.ErrShortWrite || n != 0 {
		t.Errorf("got n=%d err=%v, want 0 and io.ErrShortWrite", n, err)
	}
	if w.calls > 3 {
		t.Errorf("made %d Write calls to a stalled writer, want at most 3", w.calls)
	}
	w = &scripted{chunks: []int{4, 0, 0, 4, 0, 0, 0}, failAt: -1}
	n, err = WriteFull(w, []byte("0123456789"))
	if err != io.ErrShortWrite || n != 8 || w.sink.String() != "01234567" {
		t.Errorf("stall after progress: n=%d err=%v sink=%q, want 8, ErrShortWrite, \"01234567\"", n, err, w.sink.String())
	}
	w = &scripted{chunks: []int{0, 0, 9}, failAt: -1}
	if n, err = WriteFull(w, []byte("abc")); err != nil || n != 3 {
		t.Errorf("two stalls then progress must succeed, got n=%d err=%v", n, err)
	}
}

func TestWriteFull_empty_payload_makes_no_call(t *testing.T) {
	w := &scripted{chunks: []int{1}, failAt: -1}
	n, err := WriteFull(w, nil)
	if n != 0 || err != nil || w.calls != 0 {
		t.Errorf("n=%d err=%v calls=%d, want 0, nil, 0", n, err, w.calls)
	}
}

func TestWriteFrame_failure_at_every_position_leaves_a_prefix(t *testing.T) {
	for _, size := range []int{0, 1, 17} {
		p := payload(int64(size)+90, size)
		var ref bytes.Buffer
		if err := WriteFrame(&ref, p); err != nil {
			t.Fatal(err)
		}
		full := ref.Bytes()
		if len(full) != 2+size || int(full[0])<<8|int(full[1]) != size {
			t.Fatalf("size %d: frame is %v, want a 2-byte big-endian length then the payload", size, full)
		}
		for k := 0; k < len(full); k++ {
			w := &scripted{chunks: []int{3, 1}, failAt: k}
			err := WriteFrame(w, p)
			if !errors.Is(err, errBoom) || !bytes.Equal(w.sink.Bytes(), full[:k]) {
				t.Fatalf("size %d failAt %d: err=%v sink=%d bytes", size, k, err, w.sink.Len())
			}
		}
	}
}

func TestWriteFrame_too_large(t *testing.T) {
	w := &scripted{chunks: []int{1 << 30}, failAt: -1}
	if err := WriteFrame(w, make([]byte, 65536)); err != ErrTooLarge || w.calls != 0 {
		t.Errorf("err=%v calls=%d, want ErrTooLarge and no writes", err, w.calls)
	}
	var b bytes.Buffer
	if err := WriteFrame(&b, make([]byte, 65535)); err != nil || b.Len() != 65537 {
		t.Errorf("a 65535-byte payload is allowed: err=%v len=%d", err, b.Len())
	}
}

func TestReadFrame_truncated_at_every_byte(t *testing.T) {
	for _, size := range []int{0, 1, 5, 300} {
		p := payload(int64(size)+7, size)
		var b bytes.Buffer
		WriteFrame(&b, p)
		full := b.Bytes()
		for cut := 0; cut <= len(full); cut++ {
			for name, wrap := range map[string]func(io.Reader) io.Reader{
				"plain": func(r io.Reader) io.Reader { return r },
				"byte":  iotest.OneByteReader,
				"half":  iotest.HalfReader,
			} {
				got, err := ReadFrame(wrap(bytes.NewReader(full[:cut])))
				switch {
				case cut == 0:
					if err != io.EOF {
						t.Fatalf("%s size %d cut 0: err=%v, want io.EOF", name, size, err)
					}
				case cut < len(full):
					if err != io.ErrUnexpectedEOF {
						t.Fatalf("%s size %d cut %d of %d: err=%v, want io.ErrUnexpectedEOF", name, size, cut, len(full), err)
					}
				default:
					if err != nil || !bytes.Equal(got, p) {
						t.Fatalf("%s size %d full frame: err=%v got %d bytes", name, size, err, len(got))
					}
				}
			}
		}
	}
}

func TestReadFrame_two_frames_then_clean_eof(t *testing.T) {
	var b bytes.Buffer
	WriteFrame(&b, []byte("one"))
	WriteFrame(&b, nil)
	WriteFrame(&b, []byte("three"))
	r := iotest.OneByteReader(&b)
	for _, want := range []string{"one", "", "three"} {
		got, err := ReadFrame(r)
		if err != nil || string(got) != want {
			t.Fatalf("got %q, %v want %q", got, err, want)
		}
	}
	if _, err := ReadFrame(r); err != io.EOF {
		t.Errorf("after the last frame err=%v, want io.EOF", err)
	}
}
