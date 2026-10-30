package main

import (
	"sort"
	"strconv"
	"strings"
)

type Advisory struct{ Service, Version, ID string }
type Exposure struct {
	Port int
	ID   string
}

func Correlate(scan string, table []Advisory) []Exposure {
	out := []Exposure{}
	seen := map[Exposure]bool{}
	for _, line := range strings.Split(scan, "\n") {
		f := strings.Fields(line)
		if len(f) != 4 || f[1] != "open" || !strings.HasSuffix(f[0], "/tcp") {
			continue
		}
		port, err := strconv.Atoi(strings.TrimSuffix(f[0], "/tcp"))
		if err != nil || port < 1 || port > 65535 {
			continue
		}
		for _, a := range table {
			e := Exposure{port, a.ID}
			if a.Service == f[2] && a.Version == f[3] && !seen[e] {
				seen[e] = true
				out = append(out, e)
			}
		}
	}
	sort.Slice(out, func(i, j int) bool {
		if out[i].Port == out[j].Port {
			return out[i].ID < out[j].ID
		}
		return out[i].Port < out[j].Port
	})
	return out
}

func main() {}
