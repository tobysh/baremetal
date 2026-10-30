package main

import (
	"bytes"
	"crypto/sha256"
	"errors"
	"fmt"
	"io"
	"math/rand"
	"strings"
	"testing"
	"testing/iotest"
)

func dg(b []byte) string { s := sha256.Sum256(b); return fmt.Sprintf("sha256:%x", s) }

func TestKnownVector(t *testing.T) {
	want := "sha256:ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"
	if err := VerifyArtifact([]byte("abc"), want); err != nil {
		t.Fatalf("known SHA-256 vector of abc rejected: %v", err)
	}
	d, err := ParseDigest(want)
	if err != nil || d[0] != 0xba || d[31] != 0xad {
		t.Fatalf("ParseDigest = %x, %v", d, err)
	}
}

func TestEmptyAndRandomArtifacts(t *testing.T) {
	rng := rand.New(rand.NewSource(7))
	for i := 0; i < 40; i++ {
		data := make([]byte, rng.Intn(600))
		rng.Read(data)
		if err := VerifyArtifact(data, dg(data)); err != nil {
			t.Fatalf("len %d rejected: %v", len(data), err)
		}
		if len(data) > 0 {
			bad := append([]byte{}, data...)
			bad[rng.Intn(len(bad))] ^= 1 << uint(rng.Intn(8))
			if err := VerifyArtifact(bad, dg(data)); !errors.Is(err, ErrDigestMismatch) {
				t.Fatalf("one flipped bit: got %v, want ErrDigestMismatch", err)
			}
		}
		if err := VerifyArtifact(append(data, 0), dg(data)); !errors.Is(err, ErrDigestMismatch) {
			t.Fatalf("one appended byte: got %v, want ErrDigestMismatch", err)
		}
	}
}

func TestMalformedExpectations(t *testing.T) {
	data := []byte("release")
	good := dg(data)
	hexPart := strings.TrimPrefix(good, "sha256:")
	bad := map[string]string{
		"empty":         "",
		"no prefix":     hexPart,
		"other algo":    "sha512:" + hexPart,
		"upper prefix":  "SHA256:" + hexPart,
		"upper hex":     "sha256:" + strings.ToUpper(hexPart),
		"one upper":     "sha256:" + strings.ToUpper(hexPart[:1]) + hexPart[1:],
		"leading space": " " + good,
		"newline":       good + "\n",
		"short":         good[:len(good)-2],
		"truncated":     "sha256:" + hexPart[:8],
		"long":          good + "00",
		"non hex":       "sha256:" + hexPart[:63] + "g",
		"double prefix": "sha256:sha256:" + hexPart[:57],
		"colon only":    "sha256:",
	}
	for name, w := range bad {
		if err := VerifyArtifact(data, w); !errors.Is(err, ErrDigestFormat) {
			t.Errorf("%s (%q): got %v, want ErrDigestFormat", name, w, err)
		}
		if _, err := ParseDigest(w); !errors.Is(err, ErrDigestFormat) {
			t.Errorf("ParseDigest %s: got %v", name, err)
		}
	}
}

func TestFormatErrorBeatsMismatch(t *testing.T) {
	// A malformed expectation is reported even though the data cannot match either.
	if err := VerifyArtifact([]byte("x"), "sha256:00"); !errors.Is(err, ErrDigestFormat) {
		t.Fatalf("got %v", err)
	}
	if err := VerifyArtifact([]byte("x"), "sha256:"+strings.Repeat("0", 64)); !errors.Is(err, ErrDigestMismatch) {
		t.Fatalf("well formed but wrong: got %v", err)
	}
}

func TestReaderChunksAndLimit(t *testing.T) {
	rng := rand.New(rand.NewSource(11))
	data := make([]byte, 5000)
	rng.Read(data)
	want := dg(data)
	readers := map[string]io.Reader{
		"whole":   bytes.NewReader(data),
		"onebyte": iotest.OneByteReader(bytes.NewReader(data)),
		"half":    iotest.HalfReader(bytes.NewReader(data)),
		"dataerr": iotest.DataErrReader(bytes.NewReader(data)),
	}
	for name, r := range readers {
		if err := VerifyReader(r, int64(len(data)), want); err != nil {
			t.Errorf("%s with maxSize == len: %v", name, err)
		}
	}
	if err := VerifyReader(bytes.NewReader(data), int64(len(data))-1, want); !errors.Is(err, ErrTooLarge) {
		t.Errorf("one byte over the limit: got %v, want ErrTooLarge", err)
	}
	if err := VerifyReader(bytes.NewReader(data), 10, want); !errors.Is(err, ErrTooLarge) {
		t.Errorf("far over the limit: got %v", err)
	}
	if err := VerifyReader(bytes.NewReader(nil), 0, dg(nil)); err != nil {
		t.Errorf("empty stream, maxSize 0: %v", err)
	}
	other := append([]byte{}, data...)
	other[4999] ^= 0xff
	if err := VerifyReader(bytes.NewReader(other), 9999, want); !errors.Is(err, ErrDigestMismatch) {
		t.Errorf("changed last byte: got %v", err)
	}
}

type endless struct{ n *int64 }

func (e endless) Read(p []byte) (int, error) {
	for i := range p {
		p[i] = 'A'
	}
	*e.n += int64(len(p))
	return len(p), nil
}

func TestReaderStopsEarly(t *testing.T) {
	var n int64
	err := VerifyReader(endless{&n}, 4096, dg([]byte("A")))
	if !errors.Is(err, ErrTooLarge) {
		t.Fatalf("endless stream: got %v, want ErrTooLarge", err)
	}
	if n > 1<<20 {
		t.Fatalf("read %d bytes of an endless stream with a 4096 limit", n)
	}
}

func TestReadErrorsAndOrder(t *testing.T) {
	boom := errors.New("disk fell over")
	r := io.MultiReader(strings.NewReader("partial"), iotest.ErrReader(boom))
	if err := VerifyReader(r, 100, dg([]byte("partial"))); !errors.Is(err, boom) {
		t.Errorf("read failure: got %v, want the underlying error", err)
	}
	// want is validated before any byte is consumed.
	var n int64
	if err := VerifyReader(endless{&n}, 100, "nope"); !errors.Is(err, ErrDigestFormat) || n != 0 {
		t.Errorf("malformed want with a stream: err=%v, bytes read=%d (want ErrDigestFormat and 0)", err, n)
	}
}
