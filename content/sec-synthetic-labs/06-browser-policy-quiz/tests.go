package main

import "testing"

func TestBrowserConcepts(t *testing.T) {
	want := map[string]string{"csrf-missing": "reject", "cors-side-effect": "possible", "cors-wildcard": "blocked", "cors-explicit": "readable", "login-session": "rotate", "httponly": "no", "logout": "invalidate"}
	got := BrowserAnswers()
	for _, q := range []string{"csrf-missing", "cors-side-effect", "cors-wildcard", "cors-explicit", "login-session", "httponly", "logout"} {
		t.Run(q, func(t *testing.T) {
			if got[q] != want[q] {
				t.Fatalf("revisit the explanation for %s", q)
			}
		})
	}
}
