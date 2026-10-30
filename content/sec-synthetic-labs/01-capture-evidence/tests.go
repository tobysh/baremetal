package main

import (
	"encoding/json"
	"fmt"
	"reflect"
	"testing"
)

func TestCaptureVariants(t *testing.T) {
	for i := 0; i < 24; i++ {
		t.Run(fmt.Sprint(i), func(t *testing.T) {
			port := 2000 + i*137
			client := fmt.Sprint("client", i)
			p := []Packet{{Proto: "DNS", Query: "z.lab.test"}, {Proto: "DNS", Query: "a.lab.test"}, {Proto: "DNS", Query: "z.lab.test"},
				{Proto: "TCP", Src: client, Dst: "server", Sport: 40000 + i, Dport: port, Flags: "S"},
				{Proto: "TCP", Src: "server", Dst: client, Sport: port, Dport: 40000 + i, Flags: "SA"},
				{Proto: "TCP", Src: client, Dst: "server", Sport: 40000 + i, Dport: port, Flags: "A"},
				{Proto: "TCP", Src: client, Dst: "server", Sport: 40000 + i, Dport: port, Flags: "PA", Payload: "POST /store HTTP/1.1\r\nx-lab-transfer: archive\r\n\r\n"}}
			if i%3 == 0 {
				p[4].Src = "wrong-client"
			}
			want := port
			if i%3 == 0 {
				want = 0
			}
			raw, _ := json.Marshal(p)
			got, err := AnalyzeCapture(string(raw))
			if err != nil || got.Port != want || !reflect.DeepEqual(got.Queries, []string{"a.lab.test", "z.lab.test"}) {
				t.Fatalf("got %+v, %v; want port %d", got, err, want)
			}
		})
	}
}
func TestCaptureEdges(t *testing.T) {
	if _, err := AnalyzeCapture("{"); err == nil {
		t.Fatal("malformed JSON accepted")
	}
	got, err := AnalyzeCapture("[]")
	if err != nil || got.Port != 0 || len(got.Queries) != 0 {
		t.Fatal(got, err)
	}
	// A body marker is not a header, and a GET is not the transfer.
	for _, payload := range []string{"GET / HTTP/1.1\r\nX-Lab-Transfer: archive\r\n\r\n", "POST / HTTP/1.1\r\n\r\nX-Lab-Transfer: archive"} {
		p := []Packet{{Proto: "TCP", Src: "a", Dst: "b", Sport: 1, Dport: 2, Flags: "S"}, {Proto: "TCP", Src: "b", Dst: "a", Sport: 2, Dport: 1, Flags: "SA"}, {Proto: "TCP", Src: "a", Dst: "b", Sport: 1, Dport: 2, Flags: "A"}, {Proto: "TCP", Src: "a", Dst: "b", Sport: 1, Dport: 2, Payload: payload}}
		b, _ := json.Marshal(p)
		got, err := AnalyzeCapture(string(b))
		if err != nil || got.Port != 0 {
			t.Fatal(got, err)
		}
	}
}
