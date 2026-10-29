package main

import (
	"context"
	"testing"
	"time"
)

func TestRequestID_roundtrip(t *testing.T) {
	if _, ok := RequestID(context.Background()); ok {
		t.Error("RequestID on an empty context reported ok")
	}
	for _, id := range []string{"abc", "", "r-99"} {
		got, ok := RequestID(WithRequestID(context.Background(), id))
		if !ok || got != id {
			t.Errorf("RequestID = %q, %v; want %q, true", got, ok, id)
		}
	}
}

func TestKeys_do_not_collide_with_plain_strings(t *testing.T) {
	ctx := context.WithValue(context.Background(), "request_id", "intruder")
	ctx = context.WithValue(ctx, "requestID", "intruder")
	ctx = context.WithValue(ctx, "id", "intruder")
	ctx = context.WithValue(ctx, "user", "intruder")
	ctx = context.WithValue(ctx, "User", "intruder")
	ctx = context.WithValue(ctx, "name", "intruder")
	if id, ok := RequestID(ctx); ok {
		t.Errorf("RequestID found %q set by a plain string key; use your own key type", id)
	}
	if u := User(ctx); u != "anonymous" {
		t.Errorf("User = %q; a plain string key must not be visible", u)
	}
	ctx = WithRequestID(ctx, "mine")
	if id, _ := RequestID(ctx); id != "mine" {
		t.Errorf("RequestID = %q; want mine", id)
	}
	if got := ctx.Value("request_id"); got != "intruder" {
		t.Errorf("your WithRequestID overwrote someone else's string key (%v)", got)
	}
}

func TestUser_default_and_override(t *testing.T) {
	ctx := context.Background()
	if u := User(ctx); u != "anonymous" {
		t.Errorf("User(empty) = %q; want anonymous", u)
	}
	ctx = WithUser(ctx, "ada")
	if u := User(ctx); u != "ada" {
		t.Errorf("User = %q; want ada", u)
	}
	ctx = WithUser(ctx, "grace")
	if u := User(ctx); u != "grace" {
		t.Errorf("User after override = %q; want grace", u)
	}
}

func TestValues_are_independent(t *testing.T) {
	ctx := WithUser(context.Background(), "ada")
	ctx = WithRequestID(ctx, "r1")
	id, ok := RequestID(ctx)
	if !ok || id != "r1" || User(ctx) != "ada" {
		t.Errorf("values interfered: id=%q ok=%v user=%q", id, ok, User(ctx))
	}
}

func TestDescribe(t *testing.T) {
	cases := []struct {
		ctx  context.Context
		want string
	}{
		{context.Background(), "req=- user=anonymous"},
		{WithRequestID(context.Background(), "r7"), "req=r7 user=anonymous"},
		{WithUser(context.Background(), "bob"), "req=- user=bob"},
		{WithUser(WithRequestID(context.Background(), "x-1"), "eve"), "req=x-1 user=eve"},
	}
	for _, c := range cases {
		if got := Describe(c.ctx); got != c.want {
			t.Errorf("Describe = %q; want %q", got, c.want)
		}
	}
}

func TestDetach_keeps_values_drops_cancellation(t *testing.T) {
	parent, cancel := context.WithTimeout(WithUser(WithRequestID(context.Background(), "r5"), "ada"), time.Hour)
	d := Detach(parent)
	cancel()
	if parent.Err() == nil {
		t.Fatal("test setup: parent should be cancelled")
	}
	if d.Err() != nil {
		t.Errorf("detached context has Err %v; want nil", d.Err())
	}
	if _, ok := d.Deadline(); ok {
		t.Error("detached context still has a deadline")
	}
	select {
	case <-d.Done():
		t.Error("detached context's Done channel was closed")
	default:
	}
	if got := Describe(d); got != "req=r5 user=ada" {
		t.Errorf("Describe(detached) = %q; values were lost", got)
	}
}

func TestCancellation_flows_down_only(t *testing.T) {
	parent, cancelParent := context.WithCancel(context.Background())
	child, cancelChild := context.WithCancel(WithUser(parent, "x"))
	sibling, cancelSibling := context.WithCancel(parent)
	defer cancelSibling()
	cancelChild()
	if parent.Err() != nil || sibling.Err() != nil {
		t.Error("cancelling a child must not cancel its parent or siblings")
	}
	if child.Err() == nil {
		t.Error("child should be cancelled")
	}
	cancelParent()
	<-sibling.Done()
}
