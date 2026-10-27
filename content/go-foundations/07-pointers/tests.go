package main

import (
	"reflect"
	"testing"
)

func toSlice(n *Node) []int {
	out := []int{}
	for i := 0; n != nil && i < 1000; i++ {
		out = append(out, n.Value)
		n = n.Next
	}
	return out
}

func fromSlice(xs ...int) *Node {
	var head *Node
	for i := len(xs) - 1; i >= 0; i-- {
		head = &Node{Value: xs[i], Next: head}
	}
	return head
}

func TestSwap_exchanges_values(t *testing.T) {
	a, b := 3, 7
	Swap(&a, &b)
	if a != 7 || b != 3 {
		t.Errorf("after Swap(&a, &b) with a=3, b=7: a=%d, b=%d; want a=7, b=3", a, b)
	}
}

func TestIncrement_changes_the_callers_variable(t *testing.T) {
	n := 41
	Increment(&n)
	if n != 42 {
		t.Errorf("after Increment(&n) with n=41, n=%d; want 42", n)
	}
}

func TestIncrement_nil_is_safe(t *testing.T) {
	Increment(nil)
}

func TestPush_onto_empty_list(t *testing.T) {
	got := toSlice(Push(nil, 5))
	if !reflect.DeepEqual(got, []int{5}) {
		t.Errorf("Push(nil, 5) = %v; want [5]", got)
	}
}

func TestPush_puts_value_first(t *testing.T) {
	got := toSlice(Push(fromSlice(2, 3), 1))
	if !reflect.DeepEqual(got, []int{1, 2, 3}) {
		t.Errorf("Push(list [2 3], 1) = %v; want [1 2 3]", got)
	}
}

func TestReverse_several_nodes(t *testing.T) {
	got := toSlice(Reverse(fromSlice(1, 2, 3, 4)))
	if !reflect.DeepEqual(got, []int{4, 3, 2, 1}) {
		t.Errorf("Reverse([1 2 3 4]) = %v; want [4 3 2 1]", got)
	}
}

func TestReverse_empty_and_single(t *testing.T) {
	if Reverse(nil) != nil {
		t.Errorf("Reverse(nil) should be nil")
	}
	got := toSlice(Reverse(fromSlice(9)))
	if !reflect.DeepEqual(got, []int{9}) {
		t.Errorf("Reverse([9]) = %v; want [9]", got)
	}
}

func TestReverse_reuses_the_same_nodes(t *testing.T) {
	a := fromSlice(1, 2, 3)
	last := a.Next.Next
	if got := Reverse(a); got != last {
		t.Errorf("Reverse should return the old last node as the new head (relink, don't allocate)")
	}
}
