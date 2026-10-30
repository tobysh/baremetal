package main

import (
	"encoding/json"
	"fmt"
	"reflect"
	"testing"
)

func cutJSON(edges []Edge, extra ...string) string {
	ids := map[string]bool{}
	for _, e := range edges {
		ids[e.From] = true
		ids[e.To] = true
	}
	for _, id := range extra {
		ids[id] = true
	}
	d := Directory{Edges: edges}
	for id := range ids {
		d.Nodes = append(d.Nodes, Node{ID: id})
	}
	b, _ := json.Marshal(d)
	return string(b)
}
func TestCutParallelRoutes(t *testing.T) {
	for i := 0; i < 12; i++ {
		a := fmt.Sprint("a", i)
		b := fmt.Sprint("b", i)
		edges := []Edge{{"start", b, "AdminTo"}, {b, "goal", "HasSession"}, {"start", a, "AdminTo"}, {a, "goal", "HasSession"}, {a, "start", "MemberOf"}, {"start", a, "AdminTo"}, {"start", "goal", "CanRead"}}
		got, err := RemediationCut(cutJSON(edges), "start", "goal")
		want := []Edge{{a, "goal", "HasSession"}, {b, "goal", "HasSession"}}
		if err != nil || !reflect.DeepEqual(got, want) {
			t.Fatalf("got %v %v want %v", got, err, want)
		}
	}
}
func TestCutSharedBottleneck(t *testing.T) {
	edges := []Edge{{"s", "a", "MemberOf"}, {"s", "b", "MemberOf"}, {"a", "x", "AdminTo"}, {"b", "x", "AdminTo"}, {"x", "g", "GenericAll"}}
	got, err := RemediationCut(cutJSON(edges), "s", "g")
	if err != nil || !reflect.DeepEqual(got, []Edge{{"x", "g", "GenericAll"}}) {
		t.Fatal(got, err)
	}
}
func TestCutUnreachableAndImpossible(t *testing.T) {
	got, err := RemediationCut(cutJSON(nil, "s", "g"), "s", "g")
	if err != nil || len(got) != 0 {
		t.Fatal(got, err)
	}
	for _, edges := range [][]Edge{{{"s", "g", "MemberOf"}}, {{"s", "g", "MemberOf"}, {"s", "x", "AdminTo"}, {"x", "g", "HasSession"}}} {
		if _, err := RemediationCut(cutJSON(edges), "s", "g"); err == nil {
			t.Fatal("fixed route cannot be cut")
		}
	}
	if _, err := RemediationCut(cutJSON(nil, "s"), "s", "s"); err == nil {
		t.Fatal("zero-length route cannot be cut")
	}
}
func TestCutValidationAndLimit(t *testing.T) {
	for _, raw := range []string{"{", `{"Nodes":[{"ID":"s"},{"ID":"s"}]}`, `{"Nodes":[{"ID":"s"},{"ID":"g"}],"Edges":[{"From":"s","To":"missing","Kind":"Ignored"}]}`} {
		if _, err := RemediationCut(raw, "s", "g"); err == nil {
			t.Fatal("invalid graph accepted")
		}
	}
	edges := []Edge{}
	for i := 0; i < 17; i++ {
		edges = append(edges, Edge{"s", fmt.Sprint(i), "AdminTo"})
	}
	if _, err := RemediationCut(cutJSON(edges, "g"), "s", "g"); err == nil {
		t.Fatal("candidate limit ignored")
	}
}
func TestCutDuplicateRelationship(t *testing.T) {
	edges := []Edge{{"s", "g", "GenericAll"}, {"s", "g", "GenericAll"}}
	got, err := RemediationCut(cutJSON(edges), "s", "g")
	if err != nil || !reflect.DeepEqual(got, []Edge{{"s", "g", "GenericAll"}}) {
		t.Fatal(got, err)
	}
}
