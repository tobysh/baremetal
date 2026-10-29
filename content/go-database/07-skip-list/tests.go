package main

import (
	"bytes"
	"fmt"
	"math/rand"
	"reflect"
	"sort"
	"testing"
)

func TestPut_Get_and_overwrite(t *testing.T) {
	s := NewSkipList(1)
	s.Put("b", []byte("2"))
	s.Put("a", []byte("1"))
	s.Put("c", []byte("3"))
	s.Put("b", []byte("two"))
	if s.Len() != 3 {
		t.Errorf("Len = %d; want 3", s.Len())
	}
	for k, want := range map[string]string{"a": "1", "b": "two", "c": "3"} {
		if v, ok := s.Get(k); !ok || string(v) != want {
			t.Errorf("Get(%q) = %q, %v; want %q", k, v, ok, want)
		}
	}
	if v, ok := s.Get("zzz"); ok || v != nil {
		t.Errorf("Get of a missing key = %q, %v", v, ok)
	}
}

func TestPut_copies_the_value(t *testing.T) {
	s := NewSkipList(1)
	buf := []byte("hello")
	s.Put("k", buf)
	buf[0] = 'J'
	if v, _ := s.Get("k"); string(v) != "hello" {
		t.Errorf("stored value changed to %q", v)
	}
}

func TestKeys_in_ascending_order(t *testing.T) {
	s := NewSkipList(7)
	for _, k := range []string{"pear", "apple", "fig", "kiwi", "banana", "cherry"} {
		s.Put(k, nil)
	}
	want := []string{"apple", "banana", "cherry", "fig", "kiwi", "pear"}
	if got := s.Keys(); !reflect.DeepEqual(got, want) {
		t.Errorf("Keys = %v; want %v", got, want)
	}
	if got := NewSkipList(1).Keys(); got == nil || len(got) != 0 {
		t.Errorf("Keys of an empty list = %#v; want an empty, non-nil slice", got)
	}
}

func TestDelete_unlinks_the_key(t *testing.T) {
	s := NewSkipList(3)
	for i := 0; i < 50; i++ {
		s.Put(fmt.Sprintf("k%02d", i), []byte{byte(i)})
	}
	for i := 0; i < 50; i += 2 {
		if !s.Delete(fmt.Sprintf("k%02d", i)) {
			t.Errorf("Delete(k%02d) = false", i)
		}
	}
	if s.Delete("k00") || s.Delete("nothing") {
		t.Errorf("Delete of a missing key returned true")
	}
	if s.Len() != 25 || len(s.Keys()) != 25 {
		t.Errorf("Len = %d, len(Keys) = %d; want 25", s.Len(), len(s.Keys()))
	}
	for i := 0; i < 50; i++ {
		_, ok := s.Get(fmt.Sprintf("k%02d", i))
		if ok != (i%2 == 1) {
			t.Errorf("Get(k%02d) present = %v", i, ok)
		}
	}
	for i := 1; i < 50; i += 2 {
		s.Delete(fmt.Sprintf("k%02d", i))
	}
	if s.Len() != 0 || len(s.Keys()) != 0 || s.Height() != 1 {
		t.Errorf("after deleting everything: Len %d, keys %d, Height %d; want 0, 0, 1", s.Len(), len(s.Keys()), s.Height())
	}
	s.Put("again", []byte("x"))
	if v, ok := s.Get("again"); !ok || string(v) != "x" {
		t.Errorf("list is unusable after emptying")
	}
}

func TestSeek_finds_the_first_key_at_or_after(t *testing.T) {
	s := NewSkipList(5)
	for _, k := range []string{"b", "d", "f", "h"} {
		s.Put(k, []byte("v"+k))
	}
	cases := []struct{ seek, key string; ok bool }{
		{"", "b", true}, {"a", "b", true}, {"b", "b", true}, {"c", "d", true},
		{"f", "f", true}, {"g", "h", true}, {"h", "h", true}, {"i", "", false},
	}
	for _, c := range cases {
		k, v, ok := s.Seek(c.seek)
		if k != c.key || ok != c.ok || (ok && string(v) != "v"+c.key) {
			t.Errorf("Seek(%q) = %q, %q, %v; want %q, ok=%v", c.seek, k, v, ok, c.key, c.ok)
		}
	}
	if _, _, ok := NewSkipList(1).Seek("a"); ok {
		t.Errorf("Seek on an empty list found something")
	}
}

func TestRandom_operations_match_a_map(t *testing.T) {
	rng := rand.New(rand.NewSource(99))
	s := NewSkipList(42)
	model := map[string]string{}
	for step := 0; step < 6000; step++ {
		k := fmt.Sprintf("key%03d", rng.Intn(400))
		switch rng.Intn(3) {
		case 0, 1:
			v := fmt.Sprint(step)
			s.Put(k, []byte(v))
			model[k] = v
		default:
			_, had := model[k]
			if got := s.Delete(k); got != had {
				t.Fatalf("step %d: Delete(%s) = %v; want %v", step, k, got, had)
			}
			delete(model, k)
		}
		if step%500 == 0 {
			probe := fmt.Sprintf("key%03d", rng.Intn(400))
			v, ok := s.Get(probe)
			if mv, mok := model[probe]; ok != mok || string(v) != mv {
				t.Fatalf("step %d: Get(%s) = %q, %v; want %q, %v", step, probe, v, ok, mv, mok)
			}
		}
	}
	keys := make([]string, 0, len(model))
	for k := range model {
		keys = append(keys, k)
	}
	sort.Strings(keys)
	if got := s.Keys(); !reflect.DeepEqual(got, keys) || s.Len() != len(keys) {
		t.Fatalf("final keys differ from the model (%d vs %d)", len(got), len(keys))
	}
	for i := 0; i < 200; i++ {
		probe := fmt.Sprintf("key%03d", rng.Intn(420))
		idx := sort.SearchStrings(keys, probe)
		k, v, ok := s.Seek(probe)
		if idx == len(keys) {
			if ok {
				t.Fatalf("Seek(%s) found %q; want nothing", probe, k)
			}
			continue
		}
		if !ok || k != keys[idx] || !bytes.Equal(v, []byte(model[keys[idx]])) {
			t.Fatalf("Seek(%s) = %q, %q, %v; want %q", probe, k, v, ok, keys[idx])
		}
	}
}

func TestHeight_grows_logarithmically(t *testing.T) {
	s := NewSkipList(11)
	if s.Height() != 1 {
		t.Errorf("empty list Height = %d; want 1", s.Height())
	}
	for i := 0; i < 2000; i++ {
		s.Put(fmt.Sprintf("%05d", i), nil)
	}
	h := s.Height()
	if h < 5 || h > maxHeight {
		t.Errorf("Height = %d for 2000 keys; want between 5 and %d. Is randomHeight giving more than one level?", h, maxHeight)
	}
}

func TestPut_100000_ascending_keys_is_fast(t *testing.T) {
	s := NewSkipList(2024)
	for i := 0; i < 100000; i++ {
		s.Put(fmt.Sprintf("%08d", i), nil)
	}
	if s.Len() != 100000 {
		t.Fatalf("Len = %d", s.Len())
	}
	if _, ok := s.Get("00050000"); !ok {
		t.Errorf("key 50000 not found")
	}
	if k, _, ok := s.Seek("00099999"); !ok || k != "00099999" {
		t.Errorf("Seek(last) = %q, %v", k, ok)
	}
}
