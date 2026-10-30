package main

import (
	"crypto/ed25519"
	"crypto/sha256"
	"errors"
	"fmt"
	"strings"
	"testing"
)

func key(name string) (ed25519.PrivateKey, ed25519.PublicKey) {
	seed := make([]byte, ed25519.SeedSize)
	copy(seed, name)
	p := ed25519.NewKeyFromSeed(seed)
	return p, p.Public().(ed25519.PublicKey)
}

func line(name string, data []byte) string {
	s := sha256.Sum256(data)
	return fmt.Sprintf("file %s %d sha256:%x\n", name, len(data), s)
}

type rel struct {
	pub      ed25519.PublicKey
	priv     ed25519.PrivateKey
	manifest []byte
	sig      []byte
	files    map[string][]byte
}

func mk() rel {
	priv, pub := key("tests-publisher")
	files := map[string][]byte{
		"app.bin":    []byte("\x7fTOY\x01\x02\x03 program bytes"),
		"readme.txt": []byte("a fictional readme\n"),
		"empty.dat":  {},
	}
	m := "bm-manifest-v1\n" + line("app.bin", files["app.bin"]) + line("readme.txt", files["readme.txt"]) + line("empty.dat", files["empty.dat"])
	return rel{pub, priv, []byte(m), ed25519.Sign(priv, []byte(m)), files}
}

func signed(r rel, m string) rel {
	r.manifest = []byte(m)
	r.sig = ed25519.Sign(r.priv, r.manifest)
	return r
}

func check(t *testing.T, name string, r rel, want error) {
	t.Helper()
	err := VerifyRelease(r.pub, r.manifest, r.sig, r.files)
	if want == nil && err != nil {
		t.Errorf("%s: unexpected error %v", name, err)
	}
	if want != nil && !errors.Is(err, want) {
		t.Errorf("%s: got %v, want %v", name, err, want)
	}
}

func TestGenuineRelease(t *testing.T) {
	check(t, "genuine", mk(), nil)
}

func TestSignatureFailures(t *testing.T) {
	r := mk()
	_, other := key("someone-else")
	wk := r
	wk.pub = other
	check(t, "wrong key", wk, ErrBadSignature)

	for _, n := range []int{0, 1, 31, 33} {
		sk := r
		sk.pub = make([]byte, n)
		check(t, fmt.Sprintf("%d byte key", n), sk, ErrBadSignature)
	}
	nk := r
	nk.pub = nil
	check(t, "nil key", nk, ErrBadSignature)

	ns := r
	ns.sig = nil
	check(t, "nil sig", ns, ErrBadSignature)
	ts := r
	ts.sig = r.sig[:63]
	check(t, "truncated sig", ts, ErrBadSignature)
	fl := r
	fl.sig = append([]byte{}, r.sig...)
	fl.sig[10] ^= 1
	check(t, "flipped sig bit", fl, ErrBadSignature)

	ap := r
	ap.manifest = append(append([]byte{}, r.manifest...), []byte(line("evil.bin", []byte("x")))...)
	check(t, "manifest extended after signing", ap, ErrBadSignature)
	cr := r
	cr.manifest = []byte(strings.Replace(string(r.manifest), "app.bin", "app.biN", 1))
	check(t, "name edited after signing", cr, ErrBadSignature)
}

func TestSignatureReportedBeforeSyntax(t *testing.T) {
	r := mk()
	r.manifest = []byte("garbage that is not a manifest")
	check(t, "unsigned garbage", r, ErrBadSignature)
	r.manifest = []byte("bm-manifest-v1\nfile ../x 1 sha256:zz\n")
	check(t, "unsigned hostile", r, ErrBadSignature)
}

func TestSignedButMalformedManifests(t *testing.T) {
	r := mk()
	app := r.files["app.bin"]
	good := line("app.bin", app)
	h := fmt.Sprintf("%x", sha256.Sum256(app))
	cases := map[string]string{
		"no final newline":  "bm-manifest-v1\n" + strings.TrimSuffix(good, "\n"),
		"wrong header":      "bm-manifest-v2\n" + good,
		"no header":         good,
		"empty":             "",
		"header only":       "bm-manifest-v1\n",
		"blank line":        "bm-manifest-v1\n\n" + good,
		"trailing blank":    "bm-manifest-v1\n" + good + "\n",
		"traversal":         "bm-manifest-v1\n" + fmt.Sprintf("file ../app.bin %d sha256:%s\n", len(app), h),
		"absolute":          "bm-manifest-v1\n" + fmt.Sprintf("file /app.bin %d sha256:%s\n", len(app), h),
		"subdir":            "bm-manifest-v1\n" + fmt.Sprintf("file sub/app.bin %d sha256:%s\n", len(app), h),
		"backslash":         "bm-manifest-v1\n" + fmt.Sprintf("file sub\\app.bin %d sha256:%s\n", len(app), h),
		"dotdot":            "bm-manifest-v1\n" + fmt.Sprintf("file .. %d sha256:%s\n", len(app), h),
		"dot":               "bm-manifest-v1\n" + fmt.Sprintf("file . %d sha256:%s\n", len(app), h),
		"empty name":        "bm-manifest-v1\n" + fmt.Sprintf("file  %d sha256:%s\n", len(app), h),
		"control char":      "bm-manifest-v1\n" + fmt.Sprintf("file app\x01.bin %d sha256:%s\n", len(app), h),
		"duplicate":         "bm-manifest-v1\n" + good + good,
		"duplicate differs": "bm-manifest-v1\n" + good + line("app.bin", []byte("other")),
		"plus size":         "bm-manifest-v1\n" + fmt.Sprintf("file app.bin +%d sha256:%s\n", len(app), h),
		"zero padded size":  "bm-manifest-v1\n" + fmt.Sprintf("file app.bin 0%d sha256:%s\n", len(app), h),
		"negative size":     "bm-manifest-v1\nfile app.bin -1 sha256:" + h + "\n",
		"huge size":         "bm-manifest-v1\nfile app.bin 99999999999999999999 sha256:" + h + "\n",
		"upper digest":      "bm-manifest-v1\n" + fmt.Sprintf("file app.bin %d sha256:%s\n", len(app), strings.ToUpper(h)),
		"short digest":      "bm-manifest-v1\n" + fmt.Sprintf("file app.bin %d sha256:%s\n", len(app), h[:62]),
		"no sha prefix":     "bm-manifest-v1\n" + fmt.Sprintf("file app.bin %d %s\n", len(app), h),
		"double space":      "bm-manifest-v1\n" + fmt.Sprintf("file  app.bin %d sha256:%s\n", len(app), h),
		"extra field":       "bm-manifest-v1\n" + strings.TrimSuffix(good, "\n") + " extra\n",
		"tab separated":     "bm-manifest-v1\n" + strings.ReplaceAll(good, " ", "\t"),
		"CRLF":              "bm-manifest-v1\r\n" + strings.TrimSuffix(good, "\n") + "\r\n",
		"other verb":        "bm-manifest-v1\n" + strings.Replace(good, "file", "link", 1),
	}
	for name, m := range cases {
		s := signed(r, m)
		s.files = map[string][]byte{"app.bin": app}
		check(t, name, s, ErrBadManifest)
	}
}

func TestFileProblems(t *testing.T) {
	r := mk()

	m := r
	m.files = map[string][]byte{"app.bin": r.files["app.bin"], "empty.dat": {}}
	check(t, "readme missing", m, ErrMissingFile)

	d := mk()
	d.files["readme.txt"] = []byte("a fictional readmE\n")
	check(t, "same size, one byte changed", d, ErrFileMismatch)

	s := mk()
	s.files["app.bin"] = append(append([]byte{}, s.files["app.bin"]...), 0)
	check(t, "longer file", s, ErrFileMismatch)

	e := mk()
	e.files["empty.dat"] = []byte{0}
	check(t, "empty file replaced", e, ErrFileMismatch)

	x := mk()
	x.files["backdoor.bin"] = []byte("unlisted")
	check(t, "unlisted file", x, ErrExtraFile)

	n := mk()
	n.files["App.bin"] = n.files["app.bin"]
	check(t, "case variant is a different name", n, ErrExtraFile)

	nf := mk()
	nf.files = nil
	check(t, "no files at all", nf, ErrMissingFile)
}

func TestMissingBeatsExtraAndMismatchOrder(t *testing.T) {
	r := mk()
	delete(r.files, "readme.txt")
	r.files["zzz"] = nil
	r.files["app.bin"] = []byte("tampered")
	// manifest order: app.bin first, so its mismatch is reported before the missing readme
	check(t, "first problem in manifest order", r, ErrFileMismatch)
}

func TestInputsAreNotModified(t *testing.T) {
	r := mk()
	m0 := append([]byte{}, r.manifest...)
	s0 := append([]byte{}, r.sig...)
	f0 := len(r.files)
	VerifyRelease(r.pub, r.manifest, r.sig, r.files)
	if string(m0) != string(r.manifest) || string(s0) != string(r.sig) || len(r.files) != f0 {
		t.Fatal("VerifyRelease modified its inputs")
	}
}
