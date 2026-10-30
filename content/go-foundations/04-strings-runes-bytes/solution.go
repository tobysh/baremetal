package main

import (
	"strings"
	"unicode"
)

// RuneCount returns the number of characters (runes) in s.
func RuneCount(s string) int {
	n := 0
	for range s {
		n++
	}
	return n
}

// Reverse returns s with its characters in reverse order.
func Reverse(s string) string {
	r := []rune(s)
	for i, j := 0, len(r)-1; i < j; i, j = i+1, j-1 {
		r[i], r[j] = r[j], r[i]
	}
	return string(r)
}

// Truncate returns the first n characters of s.
func Truncate(s string, n int) string {
	if n <= 0 {
		return ""
	}
	r := []rune(s)
	if len(r) <= n {
		return s
	}
	return string(r[:n])
}

// IsPalindrome ignores case and everything that is not a letter.
func IsPalindrome(s string) bool {
	var letters []rune
	for _, r := range s {
		if unicode.IsLetter(r) {
			letters = append(letters, unicode.ToLower(r))
		}
	}
	for i := 0; i < len(letters)/2; i++ {
		if letters[i] != letters[len(letters)-1-i] {
			return false
		}
	}
	return true
}

// Shift moves every ASCII letter k places along the alphabet.
func Shift(s string, k int) string {
	var b strings.Builder
	shift := rune(k%26+26) % 26
	for _, r := range s {
		switch {
		case r >= 'a' && r <= 'z':
			b.WriteRune('a' + (r-'a'+shift)%26)
		case r >= 'A' && r <= 'Z':
			b.WriteRune('A' + (r-'A'+shift)%26)
		default:
			b.WriteRune(r)
		}
	}
	return b.String()
}
