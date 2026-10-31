package main

import (
	"fmt"
	"testing"
)

func TestPublic(t *testing.T) {
	l := []Login{{100, "kim", "192.0.2.5", true, "s1"}}
	got := Attribute(l, []Request{{150, "198.51.100.9", "s1", "/a"}}, 60)
	if got[0] != (Link{0, "kim", "session"}) {
		t.Fatal(got)
	}
}

func TestScenarios(t *testing.T) {
	for i := 0; i < 20; i++ {
		u := func(n string) string { return fmt.Sprint(n, i) }
		ip := func(k int) string { return fmt.Sprintf("192.0.2.%d", (i*7+k)%250+1) }
		base := int64(5000 + i*1000)
		win := int64(30 + i*10)
		logins := []Login{
			{base, u("ann"), ip(1), true, u("sa")},
			{base + 5, u("bob"), ip(2), true, u("sb")},
			{base + 6, u("cy"), ip(2), true, u("sc")},        // shared address with bob
			{base + 7, u("dee"), ip(3), false, ""},           // failed only
			{base + 8, u("eve"), ip(4), true, u("se")},       // address used by one account
			{base + 9, u("eve"), ip(4), true, u("se2")},      // same user twice: still one
			{base - win - 5, u("fay"), ip(5), true, u("sf")}, // outside window
			{base + 500, u("gus"), ip(6), true, u("sg")},     // logs in after request
		}
		reqs := []Request{
			{base + 20, ip(9), u("sa"), "/x"},        // session from another address
			{base + 20, ip(1), "", "/y"},             // ann by address
			{base + 20, ip(2), "", "/z"},             // ambiguous
			{base + 20, ip(3), "", "/z"},             // failed login only: none
			{base + 20, ip(4), "", "/e"},             // eve once
			{base + 20, ip(1), "forged", "/f"},       // unknown session despite known address
			{base + 20, ip(5), "", "/f"},             // outside window
			{base + 20, ip(6), u("sg"), "/g"},        // session issued later
			{base + 5, ip(2), "", "/edge"},           // exactly at bob's time: bob only
			{base + 5 + win, ip(2), u("sb"), "/ok"},  // session long after
			{base - win - 5 + win, ip(5), "", "/in"}, // exactly at window edge
		}
		want := []Link{
			{0, u("ann"), "session"}, {1, u("ann"), "ip-time"}, {2, "", "ambiguous"}, {3, "", "none"},
			{4, u("eve"), "ip-time"}, {5, "", "unknown-session"}, {6, "", "none"}, {7, "", "unknown-session"},
			{8, u("bob"), "ip-time"}, {9, u("bob"), "session"}, {10, u("fay"), "ip-time"},
		}
		got := Attribute(logins, reqs, win)
		if len(got) != len(want) {
			t.Fatal(i, got)
		}
		for k := range want {
			if got[k] != want[k] {
				t.Fatalf("scenario %d request %d: got %+v want %+v", i, k, got[k], want[k])
			}
		}
	}
}
