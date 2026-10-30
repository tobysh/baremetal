package main

import (
	"bytes"
	"crypto/ed25519"
	"crypto/sha256"
	"errors"
	"fmt"
	"sort"
	"strconv"
	"strings"
)

var (
	ErrBadSignature = errors.New("bad signature")
	ErrBadManifest  = errors.New("malformed manifest")
	ErrMissingFile  = errors.New("listed file is missing")
	ErrFileMismatch = errors.New("file does not match manifest")
	ErrExtraFile    = errors.New("file not listed in manifest")
)

type entry struct {
	name   string
	size   uint64
	digest [32]byte
}

func validName(n string) bool {
	if n == "" || n == "." || n == ".." {
		return false
	}
	for i := 0; i < len(n); i++ {
		c := n[i]
		if c <= ' ' || c == 0x7f || c == '/' || c == '\\' {
			return false
		}
	}
	return true
}

func parseHexDigest(s string) ([32]byte, bool) {
	var out [32]byte
	rest, ok := strings.CutPrefix(s, "sha256:")
	if !ok || len(rest) != 64 {
		return out, false
	}
	for i := 0; i < 32; i++ {
		hi, ok1 := nib(rest[2*i])
		lo, ok2 := nib(rest[2*i+1])
		if !ok1 || !ok2 {
			return out, false
		}
		out[i] = hi<<4 | lo
	}
	return out, true
}

func nib(c byte) (byte, bool) {
	switch {
	case c >= '0' && c <= '9':
		return c - '0', true
	case c >= 'a' && c <= 'f':
		return c - 'a' + 10, true
	}
	return 0, false
}

func parseManifest(m []byte) ([]entry, error) {
	if !bytes.HasSuffix(m, []byte("\n")) {
		return nil, fmt.Errorf("%w: missing final newline", ErrBadManifest)
	}
	lines := strings.Split(strings.TrimSuffix(string(m), "\n"), "\n")
	if lines[0] != "bm-manifest-v1" {
		return nil, fmt.Errorf("%w: bad header", ErrBadManifest)
	}
	if len(lines) < 2 {
		return nil, fmt.Errorf("%w: no files", ErrBadManifest)
	}
	var out []entry
	seen := map[string]bool{}
	for i, line := range lines[1:] {
		f := strings.Split(line, " ")
		if len(f) != 4 || f[0] != "file" {
			return nil, fmt.Errorf("%w: line %d", ErrBadManifest, i+2)
		}
		if !validName(f[1]) || seen[f[1]] {
			return nil, fmt.Errorf("%w: bad or repeated name on line %d", ErrBadManifest, i+2)
		}
		n, err := strconv.ParseUint(f[2], 10, 63)
		if err != nil || strconv.FormatUint(n, 10) != f[2] {
			return nil, fmt.Errorf("%w: bad size on line %d", ErrBadManifest, i+2)
		}
		d, ok := parseHexDigest(f[3])
		if !ok {
			return nil, fmt.Errorf("%w: bad digest on line %d", ErrBadManifest, i+2)
		}
		seen[f[1]] = true
		out = append(out, entry{f[1], n, d})
	}
	return out, nil
}

func VerifyRelease(pub ed25519.PublicKey, manifest, sig []byte, files map[string][]byte) error {
	if len(pub) != ed25519.PublicKeySize || !ed25519.Verify(pub, manifest, sig) {
		return ErrBadSignature
	}
	entries, err := parseManifest(manifest)
	if err != nil {
		return err
	}
	listed := map[string]bool{}
	for _, e := range entries {
		listed[e.name] = true
		data, ok := files[e.name]
		if !ok {
			return fmt.Errorf("%w: %s", ErrMissingFile, e.name)
		}
		sum := sha256.Sum256(data)
		if uint64(len(data)) != e.size || sum != e.digest {
			return fmt.Errorf("%w: %s", ErrFileMismatch, e.name)
		}
	}
	var extra []string
	for n := range files {
		if !listed[n] {
			extra = append(extra, n)
		}
	}
	if len(extra) > 0 {
		sort.Strings(extra)
		return fmt.Errorf("%w: %s", ErrExtraFile, extra[0])
	}
	return nil
}

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
