package main

import (
	"fmt"
	"reflect"
	"testing"
)

func TestReplicatedStore(t *testing.T) {
	l := NewLeader(0, 3, 2, nil)
	f := NewFollower()
	s := NewStore()
	want := map[string]string{}
	seen := map[string]bool{}
	for i := 0; i < 80; i++ {
		c := Command{ID: fmt.Sprint(i % 61), Key: fmt.Sprint(i % 7), Value: fmt.Sprint(i)}
		l.Propose(Encode(c))
		if e := s.Apply(l.Log, l.Commit); e != nil {
			t.Fatal(e)
		}
		if s.Applied != i {
			t.Fatal("uncommitted visibility", i, s.Applied)
		}
		req := l.Request(1, 0)
		l.OnReply(1, req, f.HandleAppend(req))
		if !seen[c.ID] {
			want[c.Key] = c.Value
			seen[c.ID] = true
		}
		if e := s.Apply(l.Log, l.Commit); e != nil || !reflect.DeepEqual(s.Values, want) {
			t.Fatal("apply", i, e, s.Values, want)
		}
		restart := NewStore()
		if e := restart.Apply(l.Log, l.Commit); e != nil || !reflect.DeepEqual(restart, s) {
			t.Fatal("replay", e)
		}
		if e := s.Apply(l.Log, l.Commit); e != nil {
			t.Fatal("idempotence")
		}
	}
}
func TestAtomicValidation(t *testing.T) {
	for _, bad := range []string{"not json", Encode(Command{ID: "", Key: "k"}), Encode(Command{ID: "a"})} {
		s := NewStore()
		log := []Entry{{Cmd: Encode(Command{ID: "one", Key: "x", Value: "v"})}, {Cmd: bad}}
		if s.Apply(log, 2) == nil || len(s.Values) != 0 || len(s.Seen) != 0 || s.Applied != 0 {
			t.Fatal("partial mutation")
		}
	}
	s := NewStore()
	if s.Apply(nil, 1) == nil || s.Apply(nil, -1) == nil {
		t.Fatal("boundary")
	}
}
