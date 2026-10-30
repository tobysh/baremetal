package main

import (
	"archive/tar"
	"bytes"
	"io"
	"sort"
	"strings"
)

type Finding struct{ Path, Kind string }

func AuditTar(data []byte) ([]Finding, error) {
	r := tar.NewReader(bytes.NewReader(data))
	out := []Finding{}
	for {
		h, err := r.Next()
		if err == io.EOF {
			break
		}
		if err != nil {
			return nil, err
		}
		if h.Typeflag != tar.TypeReg && h.Typeflag != tar.TypeRegA {
			continue
		}
		add := func(kind string) { out = append(out, Finding{h.Name, kind}) }
		if h.Mode&0002 != 0 {
			add("world-writable")
		}
		if h.Uid == 0 && h.Mode&04000 != 0 && h.Mode&0111 != 0 {
			add("root-suid")
		}
		if (strings.HasSuffix(h.Name, ".key") || strings.HasSuffix(h.Name, "/shadow")) && h.Mode&0044 != 0 {
			add("exposed-secret")
		}
	}
	sort.Slice(out, func(i, j int) bool {
		if out[i].Path == out[j].Path {
			return out[i].Kind < out[j].Kind
		}
		return out[i].Path < out[j].Path
	})
	return out, nil
}

func main() {}
