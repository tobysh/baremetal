package main

type Network struct {
	Network, Broadcast, First, Last string
	Usable                          uint64
}
type Route struct{ CIDR, Hop string }

func Subnet(cidr string) (Network, error) {
	return Network{}, nil
}

func NextHop(ip string, routes []Route) string {
	return ""
}

func main() {}
