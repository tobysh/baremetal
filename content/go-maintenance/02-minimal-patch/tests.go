package main

import "testing"

func TestTitle_apostrophes_fixed(t *testing.T) {
	cases := []struct{ in, want string }{
		{"don't stop", "Don't Stop"},
		{"rock'n'roll", "Rock'n'roll"},
		{"it’s ALIVE", "It’s ALIVE"},
		{"the cat's pyjamas", "The Cat's Pyjamas"},
		{"'tis the season", "'tis the Season"},
		{"y'all and we'd", "Y'all and We'd"},
	}
	for _, c := range cases {
		if got := Title(c.in); got != c.want {
			t.Errorf("Title(%q) = %q; want %q", c.in, got, c.want)
		}
	}
}

func TestTitle_existing_behaviour_is_pinned(t *testing.T) {
	cases := []struct{ in, want string }{
		{"", ""},
		{"   ", ""},
		{"the tale of two cities", "The Tale of Two Cities"},
		{"  lots   of   space ", "Lots of Space"},
		{"a NASA story", "A NASA Story"},
		{"OF mice AND men", "OF Mice and Men"},
		{"I", "I"},
		{"well-known and/or lesser-known", "Well-Known and/Or Lesser-Known"},
		{"(draft) notes.v2", "(Draft) Notes.V2"},
		{"3rd place", "3Rd Place"},
		{"b2b sales", "B2B Sales"},
		{"the", "The"},
		{"to be or not to be", "To Be or Not to Be"},
		{"ÉCOLE de paris", "ÉCOLE De Paris"},
		{"naïve café", "Naïve Café"},
	}
	for _, c := range cases {
		if got := Title(c.in); got != c.want {
			t.Errorf("Title(%q) = %q; want %q (this behaviour must not change)", c.in, got, c.want)
		}
	}
}

func TestNameCase_still_capitalises_after_apostrophes(t *testing.T) {
	cases := []struct{ in, want string }{
		{"mary o'brien-smith", "Mary O'Brien-Smith"},
		{"D'ARCY", "D'Arcy"},
		{"jean-luc  picard", "Jean-Luc Picard"},
		{"o’neil", "O’Neil"},
		{"", ""},
	}
	for _, c := range cases {
		if got := NameCase(c.in); got != c.want {
			t.Errorf("NameCase(%q) = %q; want %q. NameCase was not part of the bug report", c.in, got, c.want)
		}
	}
}

func TestHeading_slug_truncate_unchanged(t *testing.T) {
	if got := Heading("don't stop"); got != "Don't Stop\n==========" {
		t.Errorf("Heading = %q", got)
	}
	if got := Heading("the café"); got != "The Café\n========" {
		t.Errorf("Heading counts runes, got %q", got)
	}
	for _, c := range []struct{ in, want string }{
		{"Hello, World! 2026", "hello-world-2026"},
		{"  --Leading and trailing--  ", "leading-and-trailing"},
		{"Don't Stop", "don-t-stop"},
		{"", ""},
	} {
		if got := Slug(c.in); got != c.want {
			t.Errorf("Slug(%q) = %q; want %q", c.in, got, c.want)
		}
	}
	for _, c := range []struct {
		in   string
		n    int
		want string
	}{
		{"a long sentence here", 10, "a long ..."},
		{"short", 10, "short"},
		{"abcdef", 3, "abc"},
		{"héllo wörld", 8, "héllo..."},
		{"abc", 0, "abc"[:0]},
	} {
		if got := Truncate(c.in, c.n); got != c.want {
			t.Errorf("Truncate(%q, %d) = %q; want %q", c.in, c.n, got, c.want)
		}
	}
}
