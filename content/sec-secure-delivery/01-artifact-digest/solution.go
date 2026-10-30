package main

import (
	"bytes"
	"crypto/sha256"
	"encoding/hex"
	"errors"
	"fmt"
	"io"
	"strings"
)

var (
	ErrDigestFormat   = errors.New("malformed digest")
	ErrDigestMismatch = errors.New("digest mismatch")
	ErrTooLarge       = errors.New("artifact too large")
)

func ParseDigest(s string) ([32]byte, error) {
	var out [32]byte
	rest, ok := strings.CutPrefix(s, "sha256:")
	if !ok || len(rest) != 64 {
		return out, ErrDigestFormat
	}
	for i := 0; i < len(rest); i++ {
		c := rest[i]
		if !(c >= '0' && c <= '9' || c >= 'a' && c <= 'f') {
			return out, ErrDigestFormat
		}
	}
	if _, err := hex.Decode(out[:], []byte(rest)); err != nil {
		return out, ErrDigestFormat
	}
	return out, nil
}

func VerifyArtifact(data []byte, want string) error {
	w, err := ParseDigest(want)
	if err != nil {
		return err
	}
	got := sha256.Sum256(data)
	if !bytes.Equal(got[:], w[:]) {
		return ErrDigestMismatch
	}
	return nil
}

func VerifyReader(r io.Reader, maxSize int64, want string) error {
	w, err := ParseDigest(want)
	if err != nil {
		return err
	}
	limit := maxSize
	if limit < 0 {
		limit = 0
	}
	if limit < 1<<62 {
		limit++
	}
	h := sha256.New()
	n, err := io.Copy(h, io.LimitReader(r, limit))
	if err != nil {
		return fmt.Errorf("reading artifact: %w", err)
	}
	if n > maxSize {
		return ErrTooLarge
	}
	if !bytes.Equal(h.Sum(nil), w[:]) {
		return ErrDigestMismatch
	}
	return nil
}

func main() {
	artifact := []byte("synthetic release 1.0\n")
	sum := sha256.Sum256(artifact)
	want := fmt.Sprintf("sha256:%x", sum)
	fmt.Println(VerifyArtifact(artifact, want))
	fmt.Println(VerifyArtifact([]byte("synthetic release 1.0 (edited)\n"), want))
	fmt.Println(VerifyReader(bytes.NewReader(artifact), 1024, want))
}
