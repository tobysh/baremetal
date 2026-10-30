package main

import (
	"encoding/json"
	"fmt"
	"reflect"
	"testing"
)

func TestDirectoryPaths(t *testing.T) {
	for i := 0; i < 20; i++ {
		a := fmt.Sprint("user", i)
		z := fmt.Sprint("da", i)
		d := Directory{Nodes: []Node{{ID: a}, {ID: "b"}, {ID: "c"}, {ID: "d"}, {ID: z, Label: "Domain Admins"}}, Edges: []Edge{{a, "c", "MemberOf"}, {a, "b", "GenericAll"}, {"b", "d", "AdminTo"}, {"c", "d", "AdminTo"}, {"d", z, "HasSession"}, {"d", a, "MemberOf"}, {a, z, "CanRead"}}}
		raw, _ := json.Marshal(d)
		got, err := AdminPath(string(raw), a, z)
		want := []string{a, "b", "d", z}
		if err != nil || !reflect.DeepEqual(got, want) {
			t.Fatalf("got %v %v want %v", got, err, want)
		}
		got, err = AdminPath(string(raw), z, a)
		if err != nil || len(got) != 0 {
			t.Fatal("reversed edge", got, err)
		}
		got, err = AdminPath(string(raw), a, a)
		if err != nil || !reflect.DeepEqual(got, []string{a}) {
			t.Fatal(got, err)
		}
		// A direct edge must beat a longer path, even if its ID sorts later.
		d.Edges = append(d.Edges, Edge{a, z, "GenericAll"})
		raw, _ = json.Marshal(d)
		got, err = AdminPath(string(raw), a, z)
		if err != nil || !reflect.DeepEqual(got, []string{a, z}) {
			t.Fatal("not shortest", got, err)
		}
	}
}
func TestDirectoryInvalid(t *testing.T) {
	for _, raw := range []string{"{", `{"Nodes":[{"ID":"x"},{"ID":"x"}]}`, `{"Nodes":[{"ID":"x"}],"Edges":[{"From":"x","To":"missing","Kind":"CanRead"}]}`, `{"Nodes":[{"ID":""}]}`, `{"Nodes":[]}`} {
		if _, err := AdminPath(raw, "x", "x"); err == nil {
			t.Fatal("invalid graph accepted", raw)
		}
	}
}

func TestDifferentTopologies(t *testing.T) {
	for length := 1; length < 12; length++ {
		d := Directory{}
		want := []string{}
		for i := 0; i <= length; i++ {
			id := fmt.Sprintf("node-%d-%d", length, i)
			want = append(want, id)
			d.Nodes = append(d.Nodes, Node{ID: id, Label: "untrusted label"})
			if i > 0 {
				d.Edges = append(d.Edges, Edge{want[i-1], id, []string{"MemberOf", "GenericAll", "AdminTo", "HasSession"}[i%4]}, Edge{id, want[0], "MemberOf"})
			}
		}
		raw, _ := json.Marshal(d)
		got, err := AdminPath(string(raw), want[0], want[length])
		if err != nil || !reflect.DeepEqual(got, want) {
			t.Fatal("chain", length, got, err)
		}
		// Remove the forward edge entering the goal; the graph still has cycles.
		filtered := []Edge{}
		for _, e := range d.Edges {
			if e.To != want[length] {
				filtered = append(filtered, e)
			}
		}
		d.Edges = filtered
		raw, _ = json.Marshal(d)
		got, err = AdminPath(string(raw), want[0], want[length])
		if err != nil || len(got) != 0 {
			t.Fatal("disconnected goal", got, err)
		}
	}
}
