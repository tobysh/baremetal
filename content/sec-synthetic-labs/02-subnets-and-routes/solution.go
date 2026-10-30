package main

import (
	"encoding/binary"
	"fmt"
	"net/netip"
)

type Network struct {
	Network, Broadcast, First, Last string
	Usable                          uint64
}
type Route struct{ CIDR, Hop string }

func Subnet(cidr string) (Network, error) {
	p, err := netip.ParsePrefix(cidr)
	if err != nil || !p.Addr().Is4() {
		return Network{}, fmt.Errorf("invalid IPv4 CIDR")
	}
	p = p.Masked()
	a := p.Addr().As4()
	lo := binary.BigEndian.Uint32(a[:])
	size := uint64(1) << uint(32-p.Bits())
	hi := lo + uint32(size-1)
	first, last, usable := lo, hi, size
	if p.Bits() < 31 {
		first++
		last--
		usable -= 2
	}
	addr := func(n uint32) string {
		var b [4]byte
		binary.BigEndian.PutUint32(b[:], n)
		return netip.AddrFrom4(b).String()
	}
	return Network{addr(lo), addr(hi), addr(first), addr(last), usable}, nil
}
func NextHop(ip string, routes []Route) string {
	a, err := netip.ParseAddr(ip)
	if err != nil || !a.Is4() {
		return ""
	}
	best := -1
	hop := ""
	for _, r := range routes {
		p, err := netip.ParsePrefix(r.CIDR)
		if err == nil && p.Addr().Is4() && p.Bits() > best && p.Contains(a) {
			best = p.Bits()
			hop = r.Hop
		}
	}
	return hop
}

func main() {}
