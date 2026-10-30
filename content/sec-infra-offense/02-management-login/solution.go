package main

import (
	"encoding/json"
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

func GuessManagement(lab Lab, port int, user string, passwords []string) (Finding, bool) {
	for _, password := range passwords {
		b, _ := json.Marshal(map[string]string{"user": user, "password": password})
		r := lab.Send(Request{Port: port, Method: "POST", Path: "/login", Body: string(b)})
		if r.Status != 200 || r.Token == "" {
			continue
		}
		proof := lab.Send(Request{Port: port, Method: "GET", Path: "/admin/proof", Token: r.Token})
		if proof.Status == 200 && strings.HasPrefix(proof.Body, "LAB-PROOF:") {
			return Finding{port, proof.Body}, true
		}
	}
	return Finding{}, false
}

func main() {}
