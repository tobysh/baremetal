package main

type Node struct {
	Value int
	Next  *Node
}

// Swap exchanges the values *a and *b.
func Swap(a, b *int) {
	*a, *b = *b, *a
}

// Increment adds 1 to *p (does nothing if p is nil).
func Increment(p *int) {
	if p != nil {
		*p++
	}
}

// Push returns a new list with v in front of head.
func Push(head *Node, v int) *Node {
	return &Node{Value: v, Next: head}
}

// Reverse reverses the list in place and returns the new head.
func Reverse(head *Node) *Node {
	var prev *Node
	for head != nil {
		next := head.Next
		head.Next = prev
		prev = head
		head = next
	}
	return prev
}
