package main

import (
	"fmt"
	"strings"
	"unicode"
)

var small = map[string]bool{
	"a": true, "an": true, "and": true, "of": true, "the": true, "in": true, "to": true,
}

func isAllUpper(w string) bool {
	letters := 0
	for _, r := range w {
		if unicode.IsLower(r) {
			return false
		}
		if unicode.IsUpper(r) {
			letters++
		}
	}
	return letters >= 2
}

// Title formats text as a headline.
func Title(s string) string {
	words := strings.Fields(s)
	for i, w := range words {
		words[i] = titleWord(w, i == 0)
	}
	return strings.Join(words, " ")
}

func titleWord(w string, first bool) string {
	if isAllUpper(w) {
		return w // acronym, keep as written
	}
	lw := strings.ToLower(w)
	if !first && small[lw] {
		return lw
	}
	var b strings.Builder
	up := true
	for _, r := range lw {
		if up {
			b.WriteRune(unicode.ToUpper(r))
		} else {
			b.WriteRune(r)
		}
		if r != '\'' && r != '’' {
			up = !unicode.IsLetter(r)
		}
	}
	return b.String()
}

// NameCase formats a person's name: every part after a space, hyphen or
// apostrophe starts with a capital (o'brien -> O'Brien).
func NameCase(s string) string {
	words := strings.Fields(s)
	for i, w := range words {
		words[i] = nameWord(w)
	}
	return strings.Join(words, " ")
}

func nameWord(w string) string {
	var b strings.Builder
	up := true
	for _, r := range strings.ToLower(w) {
		if up {
			b.WriteRune(unicode.ToUpper(r))
		} else {
			b.WriteRune(r)
		}
		up = !unicode.IsLetter(r)
	}
	return b.String()
}

// Heading renders a title with an underline of equal length.
func Heading(s string) string {
	t := Title(s)
	return t + "\n" + strings.Repeat("=", len([]rune(t)))
}

// Slug makes a URL-safe lower-case identifier from text.
func Slug(s string) string {
	var b strings.Builder
	dash := false
	for _, r := range strings.ToLower(s) {
		switch {
		case unicode.IsLetter(r) || unicode.IsDigit(r):
			if dash && b.Len() > 0 {
				b.WriteByte('-')
			}
			dash = false
			b.WriteRune(r)
		default:
			dash = true
		}
	}
	return b.String()
}

// Truncate shortens s to at most n runes, ending with "..." when it cut.
func Truncate(s string, n int) string {
	r := []rune(s)
	if len(r) <= n {
		return s
	}
	if n <= 3 {
		return string(r[:n])
	}
	return string(r[:n-3]) + "..."
}

func main() {
	fmt.Println(Title("the tale of two cities"))
	fmt.Println(Title("don't stop"))
	fmt.Println(NameCase("mary o'brien-smith"))
	fmt.Println(Heading("a NASA story"))
	fmt.Println(Slug("Hello, World! 2026"), Truncate("a long sentence here", 10))
}
