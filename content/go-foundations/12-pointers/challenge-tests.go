package main

import (
	"reflect"
	"testing"
)

func build(vals ...int) (*Node, []*Node) {
	var head *Node
	nodes := make([]*Node, len(vals))
	for i := len(vals) - 1; i >= 0; i-- {
		head = &Node{Value: vals[i], Next: head}
		nodes[i] = head
	}
	return head, nodes
}

func values(head *Node, limit int) []int {
	out := []int{}
	for n := head; n != nil && len(out) <= limit; n = n.Next {
		out = append(out, n.Value)
	}
	return out
}

func TestRemoveAll_middle_start_and_end(t *testing.T) {
	head, _ := build(3, 1, 3, 2, 3, 3, 4, 3)
	got := values(RemoveAll(head, 3), 20)
	if !reflect.DeepEqual(got, []int{1, 2, 4}) {
		t.Errorf("RemoveAll(.., 3) = %v; want [1 2 4]", got)
	}
}

func TestRemoveAll_everything_and_nothing(t *testing.T) {
	head, _ := build(5, 5, 5)
	if got := RemoveAll(head, 5); got != nil {
		t.Errorf("removing every node should return nil, got a list starting at %d", got.Value)
	}
	if got := RemoveAll(nil, 1); got != nil {
		t.Errorf("RemoveAll(nil, 1) should be nil")
	}
	head, nodes := build(1, 2, 3)
	got := RemoveAll(head, 9)
	if got != nodes[0] || !reflect.DeepEqual(values(got, 10), []int{1, 2, 3}) {
		t.Errorf("a value that is not there must leave the list as it was, got %v", values(got, 10))
	}
}

func TestRemoveAll_reuses_the_nodes_and_cuts_the_removed_ones_loose(t *testing.T) {
	head, nodes := build(1, 2, 1, 3)
	got := RemoveAll(head, 1)
	if got != nodes[1] || got.Next != nodes[3] || got.Next.Next != nil {
		t.Errorf("the result must be the original nodes 2 -> 3, got %v", values(got, 10))
	}
	if nodes[0].Next != nil || nodes[2].Next != nil {
		t.Errorf("removed nodes must have Next set to nil")
	}
}

func TestRotate_examples(t *testing.T) {
	cases := []struct {
		in   []int
		k    int
		want []int
	}{
		{[]int{1, 2, 3, 4, 5}, 2, []int{4, 5, 1, 2, 3}},
		{[]int{1, 2, 3, 4, 5}, 1, []int{5, 1, 2, 3, 4}},
		{[]int{1, 2, 3, 4, 5}, 4, []int{2, 3, 4, 5, 1}},
		{[]int{1, 2, 3}, 0, []int{1, 2, 3}},
		{[]int{1, 2, 3}, 3, []int{1, 2, 3}},
		{[]int{1, 2, 3}, 7, []int{3, 1, 2}},
		{[]int{1, 2}, 1000001, []int{2, 1}},
		{[]int{9}, 4, []int{9}},
	}
	for _, c := range cases {
		head, _ := build(c.in...)
		got := values(Rotate(head, c.k), 50)
		if !reflect.DeepEqual(got, c.want) {
			t.Errorf("Rotate(%v, %d) = %v; want %v (the result must also end, with no cycle)", c.in, c.k, got, c.want)
		}
	}
}

func TestRotate_empty_list(t *testing.T) {
	if got := Rotate(nil, 3); got != nil {
		t.Errorf("Rotate(nil, 3) should be nil")
	}
}

func TestRotate_relinks_without_allocating(t *testing.T) {
	head, nodes := build(1, 2, 3, 4)
	got := Rotate(head, 1)
	if got != nodes[3] || got.Next != nodes[0] || nodes[2].Next != nil {
		t.Errorf("Rotate must reuse the same nodes: want 4 -> 1 -> 2 -> 3, got %v", values(got, 10))
	}
}
