package main

import (
	"fmt"
	"math/rand"
	"sort"
	"testing"
	"time"
)

type truth struct {
	t    int64
	src  string
	msg  string
	line int
}

func TestPublicExample(t *testing.T) {
	ev, rej := BuildTimeline([]Source{{"web", 120, []string{"2026-03-04T11:02:00+01:00 GET /login"}}})
	base := time.Date(2026, 3, 4, 10, 0, 0, 0, time.UTC).Unix()
	if len(rej) != 0 || len(ev) != 1 || ev[0].Unix != base || ev[0].Message != "GET /login" || ev[0].Line != 1 {
		t.Fatal(ev, rej)
	}
}

func TestGeneratedTimelines(t *testing.T) {
	zones := []int{-8 * 3600, -5 * 3600, 0, 3600, 2 * 3600, 5*3600 + 1800, 9 * 3600}
	for seed := int64(1); seed <= 25; seed++ {
		r := rand.New(rand.NewSource(seed))
		names := []string{"auth", "fileserver", "web"}
		skews := map[string]int64{"auth": 0, "fileserver": int64(r.Intn(900) - 450), "web": int64(r.Intn(300))}
		base := int64(1780000000 + r.Intn(1000000))
		var want []truth
		srcs := make([]Source, len(names))
		for si, n := range names {
			srcs[si] = Source{Name: n, SkewSeconds: skews[n]}
			cnt := 3 + r.Intn(8)
			type raw struct {
				line string
				tr   truth
			}
			var raws []raw
			for k := 0; k < cnt; k++ {
				tt := base + int64(r.Intn(40))*30 // many collisions
				off := zones[r.Intn(len(zones))]
				msg := fmt.Sprintf("event %d  with  spaces %d", r.Intn(100), k)
				stamp := time.Unix(tt+skews[n], 0).In(time.FixedZone("", off)).Format(time.RFC3339)
				raws = append(raws, raw{stamp + " " + msg, truth{tt, n, msg, 0}})
			}
			var rejects []string
			bad := []string{"garbage", "2026-13-40T00:00:00Z msg", "2026-03-04T10:00:00Z", "2026-03-04 10:00:00 msg", "2026-03-04T10:00:00Z "}
			for _, b := range bad[:1+r.Intn(len(bad))] {
				raws = append(raws, raw{line: b})
			}
			r.Shuffle(len(raws), func(i, j int) { raws[i], raws[j] = raws[j], raws[i] })
			for i, rw := range raws {
				srcs[si].Lines = append(srcs[si].Lines, rw.line)
				if rw.tr.msg == "" {
					rejects = append(rejects, fmt.Sprintf("%s:%d", n, i+1))
				} else {
					rw.tr.line = i + 1
					want = append(want, rw.tr)
				}
			}
			_ = rejects
		}
		var wantRej []string
		for _, s := range srcs {
			for i, l := range s.Lines {
				if _, err := time.Parse(time.RFC3339, firstField(l)); err != nil || afterField(l) == "" {
					wantRej = append(wantRej, fmt.Sprintf("%s:%d", s.Name, i+1))
				}
			}
		}
		sort.Slice(want, func(a, b int) bool {
			x, y := want[a], want[b]
			if x.t != y.t {
				return x.t < y.t
			}
			if x.src != y.src {
				return x.src < y.src
			}
			return x.line < y.line
		})
		got, rej := BuildTimeline(srcs)
		if len(got) != len(want) {
			t.Fatalf("seed %d: %d events, want %d", seed, len(got), len(want))
		}
		for i := range want {
			g := got[i]
			if g.Unix != want[i].t || g.Source != want[i].src || g.Message != want[i].msg || g.Line != want[i].line {
				t.Fatalf("seed %d event %d: got %+v want %+v", seed, i, g, want[i])
			}
		}
		if fmt.Sprint(rej) != fmt.Sprint(wantRej) {
			t.Fatalf("seed %d rejected %v want %v", seed, rej, wantRej)
		}
	}
}

func firstField(l string) string {
	for i := 0; i < len(l); i++ {
		if l[i] == ' ' {
			return l[:i]
		}
	}
	return l
}
func afterField(l string) string {
	for i := 0; i < len(l); i++ {
		if l[i] == ' ' {
			return l[i+1:]
		}
	}
	return ""
}

func TestEmpty(t *testing.T) {
	ev, rej := BuildTimeline(nil)
	if len(ev) != 0 || len(rej) != 0 {
		t.Fatal(ev, rej)
	}
}
