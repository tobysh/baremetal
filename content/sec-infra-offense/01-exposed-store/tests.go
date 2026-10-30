package main

import (
	"fmt"
	"testing"
)

type storeLab struct {
	t           *testing.T
	port        int
	marker, key string
	hardened    bool
	reads       int
}

func (s *storeLab) Send(r Request) Reply {
	if r.Port != s.port && r.Port != s.port+1 {
		s.t.Fatal("out of scope", r.Port)
	}
	if r.Method != "GET" || r.Token != "" || r.Body != "" {
		s.t.Fatal("unexpected operation", r)
	}
	if r.Path == "/banner" {
		if r.Port == s.port {
			return Reply{Status: 200, Body: "labkv/1.0"}
		}
		return Reply{Status: 200, Body: "labhttp/1.0"}
	}
	if r.Port != s.port {
		s.t.Fatal("followed a decoy banner")
	}
	if r.Path == "/keys" {
		if s.hardened {
			return Reply{Status: 401}
		}
		return Reply{Status: 200, Items: []string{"public", s.key}}
	}
	if r.Path == "/value/public" {
		return Reply{Status: 200, Body: "public documentation"}
	}
	if r.Path == "/value/"+s.key {
		s.reads++
		return Reply{Status: 200, Body: s.marker}
	}
	s.t.Fatal("unexpected path", r.Path)
	return Reply{}
}
func TestExposedStore(t *testing.T) {
	for i := 0; i < 16; i++ {
		for _, hard := range []bool{false, true} {
			s := &storeLab{t: t, port: 6000 + i*11, marker: fmt.Sprint("LAB-PROOF:store-", i), key: fmt.Sprint("backup-", i), hardened: hard}
			got, ok := DiscoverStore(s, []int{s.port + 1, s.port})
			if ok == hard {
				t.Fatal("wrong verdict", i, hard, got)
			}
			if ok && (got.Port != s.port || got.Evidence != s.marker || s.reads != 1) {
				t.Fatal("no read proof", got)
			}
		}
	}
}
func TestEmptyStoreScope(t *testing.T) {
	s := &storeLab{t: t}
	got, ok := DiscoverStore(s, nil)
	if ok || got != (Finding{}) {
		t.Fatal(got, ok)
	}
}
