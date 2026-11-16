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

// scriptedStore serves a fixed set of ports and records every request so tests can check what was (not) asked.
type scriptedStore struct {
	t      *testing.T
	ports  map[int]*scriptedPort
	log    []string
	scoped map[int]bool
}
type scriptedPort struct {
	banner     string
	keysStatus int
	keys       []string
	values     map[string]string
}

func newScripted(t *testing.T, ports map[int]*scriptedPort) *scriptedStore {
	s := &scriptedStore{t: t, ports: ports, scoped: map[int]bool{}}
	for p := range ports {
		s.scoped[p] = true
	}
	return s
}
func (s *scriptedStore) Send(r Request) Reply {
	if !s.scoped[r.Port] {
		s.t.Fatal("out of scope", r.Port)
	}
	if r.Method != "GET" || r.Token != "" || r.Body != "" {
		s.t.Fatal("unexpected operation", r)
	}
	s.log = append(s.log, fmt.Sprint(r.Port, " ", r.Path))
	p := s.ports[r.Port]
	switch {
	case r.Path == "/banner":
		return Reply{Status: 200, Body: p.banner}
	case r.Path == "/keys":
		return Reply{Status: p.keysStatus, Items: p.keys}
	case len(r.Path) > 7 && r.Path[:7] == "/value/":
		if v, ok := p.values[r.Path[7:]]; ok {
			return Reply{Status: 200, Body: v}
		}
		return Reply{Status: 404}
	}
	s.t.Fatal("unexpected path", r.Path)
	return Reply{}
}
func (s *scriptedStore) asked(entry string) bool {
	for _, l := range s.log {
		if l == entry {
			return true
		}
	}
	return false
}

func TestBannerGateBlocksFurtherRequests(t *testing.T) {
	for _, banner := range []string{"labkv/2.0", "labkv/1.0 ", "LABKV/1.0", "labhttp/1.0", ""} {
		s := newScripted(t, map[int]*scriptedPort{6100: {banner: banner, keysStatus: 200, keys: []string{"k"}, values: map[string]string{"k": "LAB-PROOF:x"}}})
		got, ok := DiscoverStore(s, []int{6100})
		if ok || got != (Finding{}) {
			t.Fatalf("banner %q accepted: %+v", banner, got)
		}
		if s.asked("6100 /keys") {
			t.Fatalf("banner %q: sent /keys although the banner did not match exactly", banner)
		}
	}
}
func TestFirstProofWinsAndStopsReading(t *testing.T) {
	s := newScripted(t, map[int]*scriptedPort{6200: {banner: "labkv/1.0", keysStatus: 200, keys: []string{"a", "b", "c"}, values: map[string]string{"a": "hello", "b": "LAB-PROOF:first", "c": "LAB-PROOF:second"}}})
	got, ok := DiscoverStore(s, []int{6200})
	if !ok || got != (Finding{6200, "LAB-PROOF:first"}) {
		t.Fatalf("got %+v %v, want the first proof (key b)", got, ok)
	}
	if s.asked("6200 /value/c") {
		t.Fatal("kept reading values after the first proof")
	}
}
func TestOnlyExactPrefixCountsAsProof(t *testing.T) {
	vals := map[string]string{"a": "LAB-PROOF", "b": "lab-proof:lower", "c": "note: LAB-PROOF:inside", "d": " LAB-PROOF:space", "e": ""}
	s := newScripted(t, map[int]*scriptedPort{6300: {banner: "labkv/1.0", keysStatus: 200, keys: []string{"a", "b", "c", "d", "e"}, values: vals}})
	if got, ok := DiscoverStore(s, []int{6300}); ok || got != (Finding{}) {
		t.Fatalf("a non-proof value was accepted: %+v", got)
	}
}
func TestHardenedPortDoesNotStopTheSearch(t *testing.T) {
	s := newScripted(t, map[int]*scriptedPort{
		6400: {banner: "labkv/1.0", keysStatus: 401},
		6401: {banner: "labhttp/1.0"},
		6402: {banner: "labkv/1.0", keysStatus: 200, keys: []string{"dump"}, values: map[string]string{"dump": "LAB-PROOF:third-port"}},
	})
	got, ok := DiscoverStore(s, []int{6400, 6401, 6402})
	if !ok || got != (Finding{6402, "LAB-PROOF:third-port"}) {
		t.Fatalf("got %+v %v; the vulnerable store is on the last port", got, ok)
	}
	if s.asked("6401 /keys") {
		t.Fatal("asked a non-store port for its keys")
	}
}
func TestPortOrderDecidesWhichStoreIsReported(t *testing.T) {
	mk := func(m string) *scriptedPort {
		return &scriptedPort{banner: "labkv/1.0", keysStatus: 200, keys: []string{"k"}, values: map[string]string{"k": m}}
	}
	s := newScripted(t, map[int]*scriptedPort{6500: mk("LAB-PROOF:A"), 6501: mk("LAB-PROOF:B")})
	if got, _ := DiscoverStore(s, []int{6501, 6500}); got.Port != 6501 || got.Evidence != "LAB-PROOF:B" {
		t.Fatalf("input order ignored: %+v", got)
	}
	if s.asked("6500 /banner") {
		t.Fatal("probed a later port after finding proof")
	}
}
func TestKeyListingMustBeSuccessful(t *testing.T) {
	s := newScripted(t, map[int]*scriptedPort{6600: {banner: "labkv/1.0", keysStatus: 500, keys: []string{"k"}, values: map[string]string{"k": "LAB-PROOF:error-page"}}})
	if got, ok := DiscoverStore(s, []int{6600}); ok || got != (Finding{}) {
		t.Fatalf("used keys from a failed listing: %+v", got)
	}
	if s.asked("6600 /value/k") {
		t.Fatal("read a value after /keys failed")
	}
}
