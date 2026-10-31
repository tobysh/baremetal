package main

import (
	"strings"
	"testing"
)

func mustLoad(t *testing.T, data string) *Book {
	t.Helper()
	b, err := Load(data)
	if err != nil {
		t.Fatalf("Load(%q): %v", data, err)
	}
	return b
}

func TestLoad_mixed_versions(t *testing.T) {
	b := mustLoad(t, "# people\r\n1| Ada |ada@x.org\r\n\r\nv2|2|Grace|g@x.org|+44 20 7946 0000\nv2|3|Alan |a@x.org|\n")
	if b.Len() != 3 {
		t.Fatalf("Len = %d; want 3", b.Len())
	}
	want := []Member{
		{1, "Ada", "ada@x.org", ""},
		{2, "Grace", "g@x.org", "+44 20 7946 0000"},
		{3, "Alan ", "a@x.org", ""},
	}
	for _, w := range want {
		if m, ok := b.Get(w.ID); !ok || m != w {
			t.Errorf("Get(%d) = %+v, %v; want %+v", w.ID, m, ok, w)
		}
	}
}

func TestSave_writes_v2_in_order(t *testing.T) {
	b := mustLoad(t, "7|Zed|z@x\n3|Amy|a@x\nv2|5|Bo|b@x|123\n")
	want := "v2|7|Zed|z@x|\nv2|3|Amy|a@x|\nv2|5|Bo|b@x|123\n"
	if got := b.Save(); got != want {
		t.Errorf("Save = %q; want %q", got, want)
	}
	if got := mustLoad(t, "").Save(); got != "" {
		t.Errorf("empty book Save = %q", got)
	}
}

func TestV2_escaping_golden(t *testing.T) {
	b := &Book{}
	m := Member{ID: 9, Name: `a|b\c`, Email: "x\ny", Phone: "p\rq"}
	if err := b.Add(m); err != nil {
		t.Fatalf("Add with reserved characters: %v", err)
	}
	want := `v2|9|a\|b\\c|x\ny|p\rq` + "\n"
	if got := b.Save(); got != want {
		t.Errorf("Save = %q; want %q", got, want)
	}
	back := mustLoad(t, want)
	if g, _ := back.Get(9); g != m {
		t.Errorf("round trip = %+v; want %+v", g, m)
	}
}

func TestRoundTrip_awkward_text(t *testing.T) {
	pieces := []string{"", "a", " ", "|", `\`, `\|`, "\n", "\r\n", "é", "v2|1", "# x", `\n`}
	id := 0
	b := &Book{}
	for _, n := range pieces {
		for _, e := range pieces {
			for _, p := range pieces {
				id++
				name := "n" + n
				if err := b.Add(Member{ID: id, Name: name, Email: e, Phone: p}); err != nil {
					t.Fatalf("Add: %v", err)
				}
			}
		}
	}
	saved := b.Save()
	b2 := mustLoad(t, saved)
	if b2.Len() != b.Len() {
		t.Fatalf("Len after round trip = %d; want %d", b2.Len(), b.Len())
	}
	for i := 1; i <= id; i++ {
		x, _ := b.Get(i)
		y, ok := b2.Get(i)
		if !ok || x != y {
			t.Fatalf("member %d changed: %+v -> %+v", i, x, y)
		}
	}
	if b2.Save() != saved {
		t.Errorf("Save(Load(Save)) differs from Save")
	}
}

func TestLoad_errors(t *testing.T) {
	cases := []struct{ data, frag string }{
		{"1|Ada|a\nv3|2|Bo|b|c\n", `line 2: unsupported version "v3"`},
		{"v2|1|Ada|a\n", "line 1:"},
		{"v2|1|Ada|a|b|c\n", "line 1:"},
		{`v2|1|A\xda|a|b` + "\n", "line 1:"},
		{`v2|1|Ada|a|b\` + "\n", "line 1:"},
		{"v2|x|Ada|a|b\n", "line 1:"},
		{"v2|1||a|b\n", "line 1:"},
		{"# c\n\nabc\n", "line 3:"},
		{"1|Ada|a\nv2|1|Bo|b|\n", "line 2:"},
		{"0|Ada|a\n", "line 1:"},
	}
	for _, c := range cases {
		b, err := Load(c.data)
		if err == nil || b != nil || !strings.Contains(err.Error(), c.frag) {
			t.Errorf("Load(%q) = %v, %v; want nil book and an error containing %q", c.data, b, err, c.frag)
		}
	}
}

func TestAdd_rules(t *testing.T) {
	b := &Book{}
	if b.Add(Member{ID: 1, Name: "Ada"}) != nil {
		t.Errorf("valid Add failed")
	}
	if b.Add(Member{ID: 1, Name: "Dup"}) == nil {
		t.Errorf("duplicate id accepted")
	}
	if b.Add(Member{ID: 0, Name: "Zero"}) == nil || b.Add(Member{ID: -2, Name: "Neg"}) == nil {
		t.Errorf("non-positive id accepted")
	}
	if b.Add(Member{ID: 2, Name: "  "}) == nil {
		t.Errorf("blank name accepted")
	}
	if b.Len() != 1 {
		t.Errorf("Len = %d; want 1", b.Len())
	}
}

func TestMigrate(t *testing.T) {
	in := "# members\r\n\r\n1| Ada |ada@x\nv2|2|Bo|b@x|\n  \n# end\n3|Cy|c@x"
	want := "# members\n\nv2|1|Ada|ada@x|\nv2|2|Bo|b@x|\n  \n# end\nv2|3|Cy|c@x|"
	got, err := Migrate(in)
	if err != nil || got != want {
		t.Fatalf("Migrate = %q, %v\nwant %q", got, err, want)
	}
	again, err := Migrate(got)
	if err != nil || again != got {
		t.Errorf("Migrate is not idempotent: %q -> %q (%v)", got, again, err)
	}
	// The migrated data must load to the same members as the original.
	a, b := mustLoad(t, in), mustLoad(t, got)
	if a.Save() != b.Save() {
		t.Errorf("migration changed the members")
	}
}

func TestMigrate_errors_return_nothing(t *testing.T) {
	for _, c := range []struct{ data, frag string }{
		{"1|Ada|a\nbroken\n", "line 2:"},
		{"# x\nv9|1|a|b|c\n", "line 2:"},
		{`v2|1|a|b|\q` + "\n", "line 1:"},
	} {
		out, err := Migrate(c.data)
		if err == nil || out != "" || !strings.Contains(err.Error(), c.frag) {
			t.Errorf("Migrate(%q) = %q, %v; want empty output and an error containing %q", c.data, out, err, c.frag)
		}
	}
}
