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

func EscapeArtifact(lab Lab, port int, root, protected string) (Finding, bool) {
	depth := len(strings.Split(strings.Trim(root, "/"), "/"))
	payload := root + "/" + strings.Repeat("../", depth) + strings.TrimPrefix(protected, "/")
	reply := lab.Send(Request{Port: port, Method: "GET", Path: "/artifact", Body: payload})
	if reply.Status == 200 && strings.HasPrefix(reply.Body, "LAB-PROOF:") {
		return Finding{port, reply.Body}, true
	}
	return Finding{}, false
}

func main() {}
