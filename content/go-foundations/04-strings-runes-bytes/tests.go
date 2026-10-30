package main

import "testing"

func TestRuneCount_ascii_and_unicode(t *testing.T) {
	for _, c := range []struct {
		in   string
		want int
	}{{"", 0}, {"go", 2}, {"héllo", 5}, {"世界", 2}, {"a😀b", 3}} {
		if got := RuneCount(c.in); got != c.want {
			t.Errorf("RuneCount(%q) = %d; want %d (len is %d bytes)", c.in, got, c.want, len(c.in))
		}
	}
}

func TestReverse_values(t *testing.T) {
	for _, c := range []struct{ in, want string }{
		{"", ""}, {"a", "a"}, {"stressed", "desserts"}, {"héllo", "olléh"}, {"世界你好", "好你界世"}, {"a😀b", "b😀a"},
	} {
		if got := Reverse(c.in); got != c.want {
			t.Errorf("Reverse(%q) = %q; want %q", c.in, got, c.want)
		}
	}
}

func TestTruncate_by_characters(t *testing.T) {
	for _, c := range []struct {
		in   string
		n    int
		want string
	}{
		{"hello", 3, "hel"}, {"hello", 5, "hello"}, {"hello", 10, "hello"}, {"hello", 0, ""}, {"hello", -2, ""},
		{"héllo", 2, "hé"}, {"世界你好", 3, "世界你"}, {"", 4, ""},
	} {
		if got := Truncate(c.in, c.n); got != c.want {
			t.Errorf("Truncate(%q, %d) = %q; want %q", c.in, c.n, got, c.want)
		}
	}
}

func TestIsPalindrome_values(t *testing.T) {
	for _, c := range []struct {
		in   string
		want bool
	}{
		{"racecar", true}, {"Racecar", true}, {"A man, a plan, a canal: Panama!", true},
		{"hello", false}, {"ab", false}, {"", true}, {"123 321", true}, {"été", true},
		{"世界世", true}, {"世界", false},
	} {
		if got := IsPalindrome(c.in); got != c.want {
			t.Errorf("IsPalindrome(%q) = %v; want %v", c.in, got, c.want)
		}
	}
}

func TestShift_values(t *testing.T) {
	for _, c := range []struct {
		in   string
		k    int
		want string
	}{
		{"abc", 1, "bcd"}, {"xyz", 3, "abc"}, {"Hello, World!", 13, "Uryyb, Jbeyq!"},
		{"abc", 0, "abc"}, {"abc", 26, "abc"}, {"abc", 27, "bcd"}, {"abc", -1, "zab"},
		{"Zebra", -30, "Vaxnw"}, {"héllo 世界", 1, "iémmp 世界"},
	} {
		if got := Shift(c.in, c.k); got != c.want {
			t.Errorf("Shift(%q, %d) = %q; want %q", c.in, c.k, got, c.want)
		}
	}
}
