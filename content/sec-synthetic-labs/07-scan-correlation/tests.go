package main

import (
	"fmt"
	"reflect"
	"testing"
)

func TestScanVariants(t *testing.T) {
	for i := 0; i < 24; i++ {
		port := 3000 + i
		v := fmt.Sprint("1.", i)
		scan := fmt.Sprintf("PORT STATE SERVICE VERSION\n%d/tcp open labsvc %s\n%d/tcp open labsvc %s\n99/udp open labsvc %s\n22/tcp closed labsvc %s\n0/tcp open labsvc %s\n70000/tcp open labsvc %s\n443/tcp open labsvc patched\n", port, v, port, v, v, v, v, v)
		table := []Advisory{{"labsvc", v, "LAB-B"}, {"labsvc", v, "LAB-A"}, {"other", v, "LAB-C"}}
		want := []Exposure{{port, "LAB-A"}, {port, "LAB-B"}}
		got := Correlate(scan, table)
		if !reflect.DeepEqual(got, want) {
			t.Fatalf("got %+v want %+v", got, want)
		}
	}
}
func TestScanNoMatches(t *testing.T) {
	if len(Correlate("header\n22/tcp filtered labssh 1.0\n22/tcp open labssh 1.0 extra", []Advisory{{"labssh", "1.0", "LAB-001"}})) != 0 {
		t.Fatal("false positive")
	}
}
