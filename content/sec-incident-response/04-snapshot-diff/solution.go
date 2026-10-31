package main

import "sort"

type File struct {
	Path, Hash, Owner string
	Mode              uint32
	MTime, CTime      int64
}

type Change struct {
	Path, Kind string
	Timestomp  bool
}

func Diff(base, now []File, allowed map[string]string) []Change {
	b := map[string]File{}
	n := map[string]File{}
	paths := map[string]bool{}
	for _, f := range base {
		b[f.Path] = f
		paths[f.Path] = true
	}
	for _, f := range now {
		n[f.Path] = f
		paths[f.Path] = true
	}
	var keys []string
	for p := range paths {
		keys = append(keys, p)
	}
	sort.Strings(keys)
	out := []Change{}
	for _, p := range keys {
		o, hasB := b[p]
		c, hasN := n[p]
		kind := ""
		switch {
		case hasB && !hasN:
			out = append(out, Change{p, "removed", false})
			continue
		case !hasB:
			kind = "added"
		case o.Hash != c.Hash:
			kind = "content"
		case o.Mode != c.Mode || o.Owner != c.Owner:
			kind = "meta"
		default:
			continue
		}
		if (kind == "added" || kind == "content") && allowed[p] != "" && allowed[p] == c.Hash {
			if hasB && (o.Mode != c.Mode || o.Owner != c.Owner) {
				kind = "meta"
			} else {
				continue
			}
		}
		st := c.MTime > c.CTime || (kind == "content" && c.MTime == o.MTime)
		out = append(out, Change{p, kind, st})
	}
	return out
}

func main() {}
