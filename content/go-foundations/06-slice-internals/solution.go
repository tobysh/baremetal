package main

// SafeAppend returns s plus v without ever writing into s's array.
func SafeAppend(s []int, v int) []int {
	out := make([]int, len(s), len(s)+1)
	copy(out, s)
	return append(out, v)
}

// Clone returns an independent copy of s.
func Clone(s []int) []int {
	if s == nil {
		return nil
	}
	out := make([]int, len(s))
	copy(out, s)
	return out
}

// Window returns s[from:to] whose capacity ends at to.
func Window(s []int, from, to int) []int {
	return s[from:to:to]
}

// RemoveAt removes the element at index i, in place, keeping order.
func RemoveAt(s []int, i int) []int {
	return append(s[:i], s[i+1:]...)
}
