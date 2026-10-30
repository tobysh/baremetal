package main

import (
	"bytes"
	"crypto/sha256"
	"errors"
	"fmt"
	"io"
)

var (
	ErrDigestFormat   = errors.New("malformed digest")
	ErrDigestMismatch = errors.New("digest mismatch")
	ErrTooLarge       = errors.New("artifact too large")
)

func ParseDigest(s string) ([32]byte, error) {
	return [32]byte{}, nil
}

func VerifyArtifact(data []byte, want string) error {
	return nil
}

func VerifyReader(r io.Reader, maxSize int64, want string) error {
	return nil
}

// Try it with Run.
func main() {
	artifact := []byte("synthetic release 1.0\n")
	sum := sha256.Sum256(artifact)
	want := fmt.Sprintf("sha256:%x", sum)
	fmt.Println(VerifyArtifact(artifact, want))
	fmt.Println(VerifyArtifact([]byte("synthetic release 1.0 (edited)\n"), want))
	fmt.Println(VerifyReader(bytes.NewReader(artifact), 1024, want))
}
