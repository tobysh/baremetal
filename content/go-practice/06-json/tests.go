package main

import (
	"reflect"
	"testing"
)

func TestEncode_uses_lowercase_keys(t *testing.T) {
	got, err := Encode([]Item{{Name: "pen", Price: 1.5, Tags: []string{"office", "blue"}}})
	want := `[{"name":"pen","price":1.5,"tags":["office","blue"]}]`
	if err != nil || got != want {
		t.Errorf("Encode = %s, %v; want %s", got, err, want)
	}
}

func TestEncode_omits_empty_tags_and_internal(t *testing.T) {
	got, _ := Encode([]Item{{Name: "cup", Price: 3, internal: 99}, {Name: "mug", Price: 4, Tags: []string{}}})
	want := `[{"name":"cup","price":3},{"name":"mug","price":4}]`
	if got != want {
		t.Errorf("Encode = %s; want %s (omit empty tags; never output the unexported field)", got, want)
	}
}

func TestEncode_empty_is_an_array_not_null(t *testing.T) {
	for _, in := range [][]Item{nil, {}} {
		got, err := Encode(in)
		if err != nil || got != "[]" {
			t.Errorf("Encode(%#v) = %q, %v; want \"[]\"", in, got, err)
		}
	}
}

func TestPretty_indents_two_spaces(t *testing.T) {
	got, err := Pretty([]Item{{Name: "a", Price: 1}})
	want := "[\n  {\n    \"name\": \"a\",\n    \"price\": 1\n  }\n]"
	if err != nil || got != want {
		t.Errorf("Pretty = %q, %v; want %q", got, err, want)
	}
	if e, _ := Pretty(nil); e != "[]" {
		t.Errorf("Pretty(nil) = %q; want \"[]\"", e)
	}
}

func TestDecode_reads_tags_and_ignores_unknown_keys(t *testing.T) {
	in := `[{"name":"pen","price":1.5,"tags":["x"],"colour":"red"},{"name":"cup"}]`
	got, err := Decode(in)
	want := []Item{{Name: "pen", Price: 1.5, Tags: []string{"x"}}, {Name: "cup"}}
	if err != nil || !reflect.DeepEqual(got, want) {
		t.Errorf("Decode = %#v, %v; want %#v", got, err, want)
	}
}

func TestDecode_errors(t *testing.T) {
	for _, bad := range []string{`[{"name":`, `{"name":"x"}`, `[{"name":"x","price":"free"}]`} {
		got, err := Decode(bad)
		if err == nil || got != nil {
			t.Errorf("Decode(%s) = %v, %v; want nil and an error", bad, got, err)
		}
	}
}

func TestTotal(t *testing.T) {
	got, err := Total(`[{"name":"a","price":1.25},{"name":"b","price":2.5},{"name":"c"}]`)
	if err != nil || got != 3.75 {
		t.Errorf("Total = %v, %v; want 3.75, nil", got, err)
	}
	if _, err := Total(`nonsense`); err == nil {
		t.Errorf("Total(nonsense) returned no error")
	}
	if got, err := Total(`[]`); err != nil || got != 0 {
		t.Errorf("Total([]) = %v, %v; want 0, nil", got, err)
	}
}
