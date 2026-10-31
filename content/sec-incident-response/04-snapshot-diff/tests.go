package main

import (
	"fmt"
	"testing"
)

func TestPublic(t *testing.T) {
	b := []File{{"/etc/x", "h1", "root", 0644, 100, 100}}
	n := []File{{"/etc/x", "h2", "root", 0644, 100, 900}}
	got := Diff(b, n, nil)
	if len(got) != 1 || got[0] != (Change{"/etc/x", "content", true}) {
		t.Fatal(got)
	}
}

func TestPlantedChanges(t *testing.T) {
	for i := 0; i < 15; i++ {
		p := func(s string) string { return fmt.Sprintf("/srv%d/%s", i, s) }
		h := func(s string) string { return fmt.Sprintf("%s-%d", s, i) }
		T := int64(10000 + i*100)
		var base, now []File
		add := func(list *[]File, f File) { *list = append(*list, f) }
		// unchanged filler, with and without time-only noise
		for k := 0; k < 5; k++ {
			f := File{p(fmt.Sprint("filler", k)), h("f"), "app", 0644, 50, 50}
			add(&base, f)
			if k%2 == 1 {
				f.MTime, f.CTime = T, T+5 // touched only
			}
			add(&now, f)
		}
		add(&base, File{p("removed"), h("r"), "root", 0600, 10, 10})
		add(&base, File{p("content-honest"), h("c0"), "root", 0644, 20, 20})
		add(&now, File{p("content-honest"), h("c1"), "root", 0644, T, T})
		add(&base, File{p("content-stomped"), h("s0"), "root", 0644, 30, 30})
		add(&now, File{p("content-stomped"), h("s1"), "root", 0644, 30, T})
		add(&now, File{p("added-future"), h("a"), "root", 0644, T + 50, T + 10})
		add(&now, File{p("added-honest"), h("a2"), "root", 0644, T, T})
		add(&base, File{p("mode"), h("m"), "root", 0644, 5, 5})
		add(&now, File{p("mode"), h("m"), "root", 0666, 5, T})
		add(&base, File{p("owner-stomp"), h("o"), "root", 0644, 5, 5})
		add(&now, File{p("owner-stomp"), h("o"), "app", 0644, T + 1, T})
		// allowlisted package update and addition
		add(&base, File{p("pkg"), h("p0"), "root", 0644, 1, 1})
		add(&now, File{p("pkg"), h("p1"), "root", 0644, T, T})
		add(&now, File{p("pkg-new"), h("pn"), "root", 0644, T, T})
		// allowlisted content but owner changed
		add(&base, File{p("pkg-owner"), h("q0"), "root", 0644, 1, 1})
		add(&now, File{p("pkg-owner"), h("q1"), "app", 0644, T, T})
		// non-allowlisted hash on an allowlisted path
		add(&base, File{p("pkg-bad"), h("b0"), "root", 0644, 1, 1})
		add(&now, File{p("pkg-bad"), h("evil"), "root", 0644, T, T})
		allowed := map[string]string{p("pkg"): h("p1"), p("pkg-new"): h("pn"), p("pkg-owner"): h("q1"), p("pkg-bad"): h("b1")}
		want := []Change{
			{p("added-future"), "added", true}, {p("added-honest"), "added", false},
			{p("content-honest"), "content", false}, {p("content-stomped"), "content", true},
			{p("mode"), "meta", false}, {p("owner-stomp"), "meta", true},
			{p("pkg-bad"), "content", false}, {p("pkg-owner"), "meta", false},
			{p("removed"), "removed", false},
		}
		// shuffle inputs deterministically
		for a := range now {
			j := (a*7 + i) % len(now)
			now[a], now[j] = now[j], now[a]
		}
		got := Diff(base, now, allowed)
		if fmt.Sprint(got) != fmt.Sprint(want) {
			t.Fatalf("case %d:\n got %v\nwant %v", i, got, want)
		}
	}
}
