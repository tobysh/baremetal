`RuneCount`: `len([]rune(s))` works, or loop with `for range s { n++ }`.
===
`Reverse`: convert with `r := []rune(s)`, swap `r[i]` and `r[j]` from both ends (`for i, j := 0, len(r)-1; i < j; i, j = i+1, j-1`), then return `string(r)`. Reversing bytes would scramble `"é"`.
===
`Truncate`: handle `n <= 0` first, then `r := []rune(s)`; if `len(r) <= n` return `s`, otherwise `string(r[:n])`.
===
`IsPalindrome`: build a `[]rune` of only the letters, lowercased (`unicode.IsLetter`, `unicode.ToLower`), then compare `letters[i]` with `letters[len-1-i]` for the first half.
===
`Shift`: use `strings.Builder` and `range s`. For a lowercase `r`, compute `'a' + (r-'a'+rune(k)%26+26)%26`. The `+26` after the first `%26` keeps negative shifts positive. Do the same with `'A'` for uppercase and write other runes as they are.
