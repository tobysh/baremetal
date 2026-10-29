package main

import (
	"fmt"
	"reflect"
	"testing"
)

func TestPut_and_Get_round_trip(t *testing.T) {
	s := NewStore()
	for i := 0; i < 20; i++ {
		s.Put(fmt.Sprintf("key%02d", i), []byte(fmt.Sprintf("value-%d", i*i)))
	}
	for i := 0; i < 20; i++ {
		v, ok := s.Get(fmt.Sprintf("key%02d", i))
		if !ok || string(v) != fmt.Sprintf("value-%d", i*i) {
			t.Errorf("Get(key%02d) = %q, %v; want value-%d, true", i, v, ok, i*i)
		}
	}
	if s.Len() != 20 {
		t.Errorf("Len = %d; want 20", s.Len())
	}
}

func TestPut_overwrites_without_growing(t *testing.T) {
	s := NewStore()
	s.Put("k", []byte("one"))
	s.Put("k", []byte("two"))
	v, _ := s.Get("k")
	if string(v) != "two" || s.Len() != 1 {
		t.Errorf("after overwrite got %q with Len %d; want two with Len 1", v, s.Len())
	}
}

func TestGet_missing_key(t *testing.T) {
	s := NewStore()
	v, ok := s.Get("nope")
	if ok || v != nil {
		t.Errorf("Get on empty store = %q, %v; want nil, false", v, ok)
	}
}

func TestEmpty_value_is_not_missing(t *testing.T) {
	s := NewStore()
	s.Put("empty", []byte{})
	s.Put("nilval", nil)
	for _, k := range []string{"empty", "nilval"} {
		v, ok := s.Get(k)
		if !ok || len(v) != 0 {
			t.Errorf("Get(%q) = %q, %v; want an empty value that is present", k, v, ok)
		}
	}
	if s.Len() != 2 {
		t.Errorf("Len = %d; want 2", s.Len())
	}
}

func TestPut_copies_the_input(t *testing.T) {
	s := NewStore()
	buf := []byte("hello")
	s.Put("k", buf)
	copy(buf, "XXXXX")
	v, _ := s.Get("k")
	if string(v) != "hello" {
		t.Errorf("the stored value changed to %q when the caller reused its buffer; Put must copy", v)
	}
}

func TestGet_returns_a_copy(t *testing.T) {
	s := NewStore()
	s.Put("k", []byte("hello"))
	v, _ := s.Get("k")
	v[0] = 'J'
	w, _ := s.Get("k")
	if string(w) != "hello" {
		t.Errorf("editing the slice returned by Get changed the store to %q; Get must copy", w)
	}
}

func TestDelete_removes_and_reports(t *testing.T) {
	s := NewStore()
	s.Put("a", []byte("1"))
	if !s.Delete("a") {
		t.Errorf("Delete of an existing key returned false")
	}
	if s.Delete("a") {
		t.Errorf("Delete of a missing key returned true")
	}
	if _, ok := s.Get("a"); ok || s.Len() != 0 {
		t.Errorf("key still present after Delete (Len %d)", s.Len())
	}
}

func TestKeys_are_sorted(t *testing.T) {
	s := NewStore()
	for _, k := range []string{"pear", "apple", "fig", "banana", "cherry"} {
		s.Put(k, []byte(k))
	}
	want := []string{"apple", "banana", "cherry", "fig", "pear"}
	if got := s.Keys(); !reflect.DeepEqual(got, want) {
		t.Errorf("Keys = %v; want %v", got, want)
	}
	s.Delete("fig")
	want = []string{"apple", "banana", "cherry", "pear"}
	if got := s.Keys(); !reflect.DeepEqual(got, want) {
		t.Errorf("Keys after Delete = %v; want %v", got, want)
	}
}

func TestKeys_empty_store(t *testing.T) {
	got := NewStore().Keys()
	if got == nil || len(got) != 0 {
		t.Errorf("Keys of an empty store = %#v; want an empty, non-nil slice", got)
	}
}
