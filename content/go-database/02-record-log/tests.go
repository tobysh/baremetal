package main

import (
	"bytes"
	"errors"
	"fmt"
	"io"
	"testing"
)

type countingWriter struct {
	buf    bytes.Buffer
	writes int
}

func (c *countingWriter) Write(p []byte) (int, error) {
	c.writes++
	return c.buf.Write(p)
}

type failWriter struct{}

func (failWriter) Write(p []byte) (int, error) { return 0, errors.New("disk full") }

func TestAppendRecord_exact_bytes(t *testing.T) {
	var buf bytes.Buffer
	if err := AppendRecord(&buf, []byte("ab"), []byte("xyz")); err != nil {
		t.Fatalf("AppendRecord: %v", err)
	}
	want := []byte{0, 0, 0, 2, 0, 0, 0, 3, 'a', 'b', 'x', 'y', 'z'}
	if !bytes.Equal(buf.Bytes(), want) {
		t.Errorf("record bytes = % x; want % x", buf.Bytes(), want)
	}
}

func TestAppendRecord_uses_one_write(t *testing.T) {
	cw := &countingWriter{}
	AppendRecord(cw, []byte("key"), []byte("value"))
	if cw.writes != 1 {
		t.Errorf("AppendRecord called Write %d times; want exactly 1", cw.writes)
	}
}

func TestAppendRecord_reports_write_errors(t *testing.T) {
	if err := AppendRecord(failWriter{}, []byte("k"), []byte("v")); err == nil {
		t.Errorf("AppendRecord ignored the writer's error")
	}
}

func TestAppendRecord_rejects_oversized_fields(t *testing.T) {
	var buf bytes.Buffer
	big := make([]byte, MaxField+1)
	if err := AppendRecord(&buf, big, []byte("v")); err == nil {
		t.Errorf("an oversized key was accepted")
	}
	if err := AppendRecord(&buf, []byte("k"), big); err == nil {
		t.Errorf("an oversized value was accepted")
	}
	if buf.Len() != 0 {
		t.Errorf("%d bytes were written for rejected records; want 0", buf.Len())
	}
	if err := AppendRecord(&buf, make([]byte, MaxField), nil); err != nil {
		t.Errorf("a key of exactly MaxField bytes was rejected: %v", err)
	}
}

func TestReadRecord_round_trip_many(t *testing.T) {
	var buf bytes.Buffer
	for i := 0; i < 200; i++ {
		k := []byte(fmt.Sprintf("key-%d", i))
		v := bytes.Repeat([]byte{byte(i)}, i%17)
		if err := AppendRecord(&buf, k, v); err != nil {
			t.Fatalf("AppendRecord %d: %v", i, err)
		}
	}
	for i := 0; i < 200; i++ {
		k, v, err := ReadRecord(&buf)
		if err != nil {
			t.Fatalf("ReadRecord %d: %v", i, err)
		}
		wantV := bytes.Repeat([]byte{byte(i)}, i%17)
		if string(k) != fmt.Sprintf("key-%d", i) || !bytes.Equal(v, wantV) {
			t.Fatalf("record %d = %q, %q; want key-%d and %d bytes of %d", i, k, v, i, i%17, i)
		}
	}
	if _, _, err := ReadRecord(&buf); err != io.EOF {
		t.Errorf("after the last record ReadRecord returned %v; want io.EOF", err)
	}
}

func TestReadRecord_empty_key_and_value(t *testing.T) {
	var buf bytes.Buffer
	AppendRecord(&buf, nil, nil)
	AppendRecord(&buf, []byte("k"), []byte{})
	k, v, err := ReadRecord(&buf)
	if err != nil || len(k) != 0 || len(v) != 0 {
		t.Errorf("empty record = %q, %q, %v", k, v, err)
	}
	k, v, err = ReadRecord(&buf)
	if err != nil || string(k) != "k" || len(v) != 0 {
		t.Errorf("record with empty value = %q, %q, %v", k, v, err)
	}
}

func TestReadRecord_empty_stream_is_EOF(t *testing.T) {
	if _, _, err := ReadRecord(bytes.NewReader(nil)); err != io.EOF {
		t.Errorf("empty stream: err = %v; want io.EOF", err)
	}
}

func TestReadRecord_truncated_is_unexpected_EOF(t *testing.T) {
	var buf bytes.Buffer
	AppendRecord(&buf, []byte("hello"), []byte("world"))
	full := buf.Bytes()
	for cut := 1; cut < len(full); cut++ {
		_, _, err := ReadRecord(bytes.NewReader(full[:cut]))
		if err != io.ErrUnexpectedEOF {
			t.Errorf("record cut at %d of %d bytes: err = %v; want io.ErrUnexpectedEOF", cut, len(full), err)
		}
	}
}

func TestReadRecord_rejects_huge_lengths(t *testing.T) {
	hdr := []byte{0xFF, 0xFF, 0xFF, 0xFF, 0, 0, 0, 1, 'x'}
	_, _, err := ReadRecord(bytes.NewReader(hdr))
	if err == nil || err == io.EOF || err == io.ErrUnexpectedEOF {
		t.Errorf("a 4 GB key length gave err = %v; want a different, non-nil error", err)
	}
	hdr = []byte{0, 0, 0, 1, 0x80, 0, 0, 0, 'x'}
	_, _, err = ReadRecord(bytes.NewReader(hdr))
	if err == nil || err == io.EOF || err == io.ErrUnexpectedEOF {
		t.Errorf("a 2 GB value length gave err = %v; want a different, non-nil error", err)
	}
}
