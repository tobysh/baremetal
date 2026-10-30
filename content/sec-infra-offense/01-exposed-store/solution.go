package main

import (
	"strings"
)

// Lab is a fictional in-process service. Send does not use HTTP or sockets.
type Request struct {
	Port                      int
	Method, Path, Token, Body string
}
type Reply struct {
	Status      int
	Body, Token string
	Items       []string
}
type Lab interface{ Send(Request) Reply }
type Finding struct {
	Port     int
	Evidence string
}

func DiscoverStore(lab Lab, ports []int) (Finding, bool) {
	for _, port := range ports {
		b := lab.Send(Request{Port: port, Method: "GET", Path: "/banner"})
		if b.Status != 200 || b.Body != "labkv/1.0" {
			continue
		}
		keys := lab.Send(Request{Port: port, Method: "GET", Path: "/keys"})
		if keys.Status != 200 {
			continue
		}
		for _, key := range keys.Items {
			v := lab.Send(Request{Port: port, Method: "GET", Path: "/value/" + key})
			if v.Status == 200 && strings.HasPrefix(v.Body, "LAB-PROOF:") {
				return Finding{port, v.Body}, true
			}
		}
	}
	return Finding{}, false
}

func main() {}
