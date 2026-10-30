package main

import (
	"encoding/json"
	"sort"
	"strings"
)

type Packet struct {
	Proto   string `json:"proto"`
	Src     string `json:"src"`
	Dst     string `json:"dst"`
	Sport   int    `json:"sport"`
	Dport   int    `json:"dport"`
	Flags   string `json:"flags"`
	Payload string `json:"payload"`
	Query   string `json:"query"`
}
type Evidence struct {
	Port    int
	Queries []string
}

func AnalyzeCapture(raw string) (Evidence, error) {
	var packets []Packet
	if err := json.Unmarshal([]byte(raw), &packets); err != nil {
		return Evidence{}, err
	}
	type flow struct {
		src, dst     string
		sport, dport int
	}
	states := map[flow]int{}
	names := map[string]bool{}
	out := Evidence{}
	for _, p := range packets {
		if p.Proto == "DNS" && p.Query != "" {
			names[p.Query] = true
		}
		if p.Proto != "TCP" {
			continue
		}
		f := flow{p.Src, p.Dst, p.Sport, p.Dport}
		rev := flow{p.Dst, p.Src, p.Dport, p.Sport}
		switch p.Flags {
		case "S":
			states[f] = 1
		case "SA":
			if states[rev] == 1 {
				states[rev] = 2
			}
		case "A":
			if states[f] == 2 {
				states[f] = 3
			}
		}
		lines := strings.Split(p.Payload, "\r\n")
		if out.Port != 0 || states[f] != 3 || !strings.HasPrefix(lines[0], "POST ") {
			continue
		}
		for _, line := range lines[1:] {
			if line == "" {
				break
			}
			k, v, ok := strings.Cut(line, ":")
			if ok && strings.EqualFold(k, "X-Lab-Transfer") && strings.TrimSpace(v) == "archive" {
				out.Port = p.Dport
			}
		}
	}
	for n := range names {
		out.Queries = append(out.Queries, n)
	}
	sort.Strings(out.Queries)
	return out, nil
}

func main() {}
