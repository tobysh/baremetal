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

// RemoveAll removes every node with Value v and returns the new head.
func RemoveAll(head *Node, v int) *Node {
	link := &head
	for *link != nil {
		if (*link).Value == v {
			gone := *link
			*link = gone.Next
			gone.Next = nil
		} else {
			link = &(*link).Next
		}
	}
	return head
}

// Rotate moves the last k nodes to the front and returns the new head.
func Rotate(head *Node, k int) *Node {
	if head == nil || head.Next == nil {
		return head
	}
	n := 1
	tail := head
	for tail.Next != nil {
		tail = tail.Next
		n++
	}
	k %= n
	if k == 0 {
		return head
	}
	newTail := head
	for i := 0; i < n-k-1; i++ {
		newTail = newTail.Next
	}
	newHead := newTail.Next
	newTail.Next = nil
	tail.Next = head
	return newHead
}
