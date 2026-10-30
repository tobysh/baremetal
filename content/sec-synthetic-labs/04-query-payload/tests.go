package main

import (
	"fmt"
	"strings"
	"testing"
)

type labUser struct {
	id   int
	name string
}

func toyQuery(name string, users []labUser) []int {
	sql := "SELECT id FROM users WHERE name='" + name + "'"
	out := []int{}
	for _, u := range users {
		if strings.Contains(sql, "' OR 1=1 --") || name == u.name {
			out = append(out, u.id)
		}
	}
	return out
}
func boundQuery(name string, users []labUser) []int {
	out := []int{}
	for _, u := range users {
		if name == u.name {
			out = append(out, u.id)
		}
	}
	return out
}
func TestUnauthorizedRows(t *testing.T) {
	for i := 0; i < 20; i++ {
		t.Run(fmt.Sprint(i), func(t *testing.T) {
			users := []labUser{{i*3 + 1, fmt.Sprint("member", i)}, {i*3 + 2, fmt.Sprint("admin", i)}}
			payload := InjectionPayload()
			got := toyQuery(payload, users)
			if len(got) != len(users) || got[1] != users[1].id {
				t.Fatal("payload did not return unauthorized rows")
			}
			if len(boundQuery(payload, users)) != 0 {
				t.Fatal("payload unexpectedly matched a real name")
			}
		})
	}
}
func TestOrdinaryNames(t *testing.T) {
	users := []labUser{{1, "alice"}, {2, "O'Brien"}}
	if len(toyQuery("alice", users)) != 1 || len(boundQuery("O'Brien", users)) != 1 {
		t.Fatal("toy target baseline broken")
	}
}
