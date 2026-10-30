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
