package main

import (
	"fmt"
	"path"
	"strings"
	"testing"
)

type artifactLab struct {
	t                       *testing.T
	port                    int
	root, protected, marker string
	patched                 bool
	calls                   int
	read                    bool
}

func (s *artifactLab) Send(r Request) Reply {
	s.calls++
	if s.calls > 1 || r.Port != s.port || r.Method != "GET" || r.Path != "/artifact" || r.Token != "" {
		s.t.Fatal("unexpected request")
	}
	normalized := path.Clean(r.Body)
	check := r.Body
	if s.patched {
		check = normalized
	}
	if !strings.HasPrefix(check, s.root+"/") {
		return Reply{Status: 403}
	}
	if normalized == s.protected {
		s.read = true
		return Reply{Status: 200, Body: s.marker}
	}
	return Reply{Status: 200, Body: "requested path: " + r.Body}
}
func TestArtifactTraversal(t *testing.T) {
	for i, root := range []string{"/srv/artifacts", "/lab/releases/public", "/downloads", "/a/b/c/d"} {
		for _, patched := range []bool{false, true} {
			s := &artifactLab{t: t, port: 9000 + i, root: root, protected: fmt.Sprintf("/private/seed%d.key", i), marker: fmt.Sprint("LAB-PROOF:file-", i), patched: patched}
			got, ok := EscapeArtifact(s, s.port, s.root, s.protected)
			if ok == patched || ok && (got.Port != s.port || got.Evidence != s.marker || !s.read) {
				t.Fatal(got, ok, patched)
			}
		}
	}
}
func TestReflectionIsNotProof(t *testing.T) {
	s := &artifactLab{t: t, port: 90, root: "/a", protected: "/private/actual", marker: "LAB-PROOF:actual"}
	got, ok := EscapeArtifact(s, 90, "/a", "/private/missing")
	if ok || got != (Finding{}) {
		t.Fatal("reflection accepted", got)
	}
}

// recordingArtifact remembers the raw payload so tests can inspect its shape, and answers per a mode.
type recordingArtifact struct {
	inner   *artifactLab
	mode    string // "", "decoy-marker", "forbidden-with-marker"
	payload string
}

func (s *recordingArtifact) Send(r Request) Reply {
	s.payload = r.Body
	switch s.mode {
	case "decoy-marker":
		s.inner.calls++
		return Reply{Status: 200, Body: "LAB-PROOF index of " + r.Body}
	case "forbidden-with-marker":
		s.inner.calls++
		return Reply{Status: 403, Body: s.inner.marker}
	}
	return s.inner.Send(r)
}

func TestPayloadKeepsRawPrefixAndResolvesToProtected(t *testing.T) {
	cases := []struct{ root, protected string }{
		{"/a", "/secret"}, {"/srv/app/releases", "/etc/lab-secret"}, {"/one/two/three/four/five", "/top/level/deep/file.key"},
		{"/x/y", "/x/private.key"}, {"/data", "/data-private/file"},
	}
	for i, c := range cases {
		inner := &artifactLab{t: t, port: 9100 + i, root: c.root, protected: c.protected, marker: fmt.Sprint("LAB-PROOF:p", i)}
		rec := &recordingArtifact{inner: inner}
		got, ok := EscapeArtifact(rec, inner.port, c.root, c.protected)
		if !ok || got.Evidence != inner.marker {
			t.Fatalf("%s -> %s: got %+v %v", c.root, c.protected, got, ok)
		}
		if !strings.HasPrefix(rec.payload, c.root+"/") {
			t.Fatalf("payload %q lost the raw prefix %q", rec.payload, c.root+"/")
		}
		if path.Clean(rec.payload) != c.protected {
			t.Fatalf("payload %q resolves to %q, want %q", rec.payload, path.Clean(rec.payload), c.protected)
		}
		if !strings.Contains(rec.payload, "..") {
			t.Fatalf("payload %q does not climb with ..", rec.payload)
		}
	}
}
func TestSiblingDirectoryWithSharedPrefixStillEscapes(t *testing.T) {
	// "/data" and "/data-private": a naive prefix check without the trailing slash would hide the bug.
	inner := &artifactLab{t: t, port: 9200, root: "/data", protected: "/data-private/file", marker: "LAB-PROOF:sibling"}
	if got, ok := EscapeArtifact(inner, 9200, "/data", "/data-private/file"); !ok || got.Evidence != "LAB-PROOF:sibling" {
		t.Fatal(got, ok)
	}
}
func TestHardenedTargetIsReportedClean(t *testing.T) {
	for i, root := range []string{"/a", "/srv/x/y", "/p/q/r/s"} {
		inner := &artifactLab{t: t, port: 9300 + i, root: root, protected: "/private/key", marker: "LAB-PROOF:never", patched: true}
		if got, ok := EscapeArtifact(inner, inner.port, root, "/private/key"); ok || got != (Finding{}) || inner.read {
			t.Fatalf("patched target at %s reported compromised: %+v", root, got)
		}
	}
}
func TestMarkerLookalikeBodyIsNotProof(t *testing.T) {
	inner := &artifactLab{t: t, port: 9400, root: "/srv/a", protected: "/private/k", marker: "LAB-PROOF:real"}
	rec := &recordingArtifact{inner: inner, mode: "decoy-marker"}
	if got, ok := EscapeArtifact(rec, 9400, "/srv/a", "/private/k"); ok || got != (Finding{}) {
		t.Fatalf("accepted a body without the exact LAB-PROOF: prefix: %+v", got)
	}
}
func TestRefusedReplyIsNotProofEvenWithAMarkerBody(t *testing.T) {
	inner := &artifactLab{t: t, port: 9500, root: "/srv/a", protected: "/private/k", marker: "LAB-PROOF:leak"}
	rec := &recordingArtifact{inner: inner, mode: "forbidden-with-marker"}
	if got, ok := EscapeArtifact(rec, 9500, "/srv/a", "/private/k"); ok || got != (Finding{}) {
		t.Fatalf("a 403 reply was treated as a successful read: %+v", got)
	}
}
