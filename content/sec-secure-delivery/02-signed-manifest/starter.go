package main

import (
	"crypto/ed25519"
	"crypto/sha256"
	"errors"
	"fmt"
)

var (
	ErrBadSignature = errors.New("bad signature")
	ErrBadManifest  = errors.New("malformed manifest")
	ErrMissingFile  = errors.New("listed file is missing")
	ErrFileMismatch = errors.New("file does not match manifest")
	ErrExtraFile    = errors.New("file not listed in manifest")
)

func VerifyRelease(pub ed25519.PublicKey, manifest, sig []byte, files map[string][]byte) error {
	return nil
}

// Try it with Run. The key comes from a fixed seed and exists only for this lesson.
func main() {
	seed := make([]byte, ed25519.SeedSize)
	copy(seed, "bm-lesson-02-throwaway-seed")
	priv := ed25519.NewKeyFromSeed(seed)
	pub := priv.Public().(ed25519.PublicKey)

	body := []byte("hello from the toy release\n")
	sum := sha256.Sum256(body)
	manifest := []byte(fmt.Sprintf("bm-manifest-v1\nfile hello.txt %d sha256:%x\n", len(body), sum))
	sig := ed25519.Sign(priv, manifest)
	files := map[string][]byte{"hello.txt": body}

	fmt.Println("genuine:", VerifyRelease(pub, manifest, sig, files))
	files["hello.txt"] = []byte("hello from a swapped release\n")
	fmt.Println("swapped file:", VerifyRelease(pub, manifest, sig, files))
}
