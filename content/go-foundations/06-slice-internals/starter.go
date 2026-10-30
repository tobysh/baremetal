package main

// SafeAppend returns s plus v without ever writing into s's array.
func SafeAppend(s []int, v int) []int {
	return append(s, v) // can overwrite shared memory: fix it
}

// Clone returns an independent copy of s.
func Clone(s []int) []int {
	return s
}

// Window returns s[from:to] whose capacity ends at to.
func Window(s []int, from, to int) []int {
	return s[from:to]
}

// RemoveAt removes the element at index i, in place, keeping order.
func RemoveAt(s []int, i int) []int {
	return s
}
