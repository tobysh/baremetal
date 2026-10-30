package main

import (
	"encoding/binary"
	"fmt"
	"math/rand"
	"net/netip"
	"testing"
)

func TestSubnets(t *testing.T) {
	rng := rand.New(rand.NewSource(8451))
	for bits := 0; bits <= 32; bits++ {
		for i := 0; i < 5; i++ {
			n := rng.Uint32()
			var b [4]byte
			binary.BigEndian.PutUint32(b[:], n)
			cidr := fmt.Sprintf("%s/%d", netip.AddrFrom4(b), bits)
			got, err := Subnet(cidr)
			size := uint64(1) << uint(32-bits)
			lo := uint64(n) / size * size
			hi := lo + size - 1
			first, last, count := lo, hi, size
			if bits < 31 {
				first++
				last--
				count -= 2
			}
			addr := func(v uint64) string {
				var x [4]byte
				binary.BigEndian.PutUint32(x[:], uint32(v))
				return netip.AddrFrom4(x).String()
			}
			want := Network{addr(lo), addr(hi), addr(first), addr(last), count}
			if err != nil || got != want {
				t.Fatalf("%s: got %+v %v want %+v", cidr, got, err, want)
			}
		}
	}
	for _, s := range []string{"bad", "::1/128", "10.0.0.1/33"} {
		if _, err := Subnet(s); err == nil {
			t.Fatalf("accepted %s", s)
		}
	}
}
func TestRoutes(t *testing.T) {
	r := []Route{{"0.0.0.0/0", "default"}, {"10.0.0.0/8", "wide"}, {"10.2.0.0/16", "narrow"}, {"10.2.0.0/16", "later"}, {"10.2.3.4/32", "host"}}
	for _, c := range []struct{ ip, want string }{{"10.2.3.4", "host"}, {"10.2.9.1", "narrow"}, {"10.8.0.1", "wide"}, {"192.0.2.1", "default"}, {"::1", ""}, {"bad", ""}} {
		if got := NextHop(c.ip, r); got != c.want {
			t.Fatal(c, got)
		}
	}
	if NextHop("192.0.2.1", r[1:]) != "" {
		t.Fatal("invented route")
	}
}
