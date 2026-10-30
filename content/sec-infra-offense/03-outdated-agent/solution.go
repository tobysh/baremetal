package main

import (
	"encoding/json"
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

func ExploitAgent(lab Lab, port int, marker string) (Finding, bool) {
	version := lab.Send(Request{Port: port, Method: "GET", Path: "/version"})
	if version.Status != 200 || version.Body != "labagent/0.9" {
		return Finding{}, false
	}
	body, _ := json.Marshal(map[string]string{"label": "nightly; emit " + marker})
	lab.Send(Request{Port: port, Method: "POST", Path: "/job", Body: string(body)})
	proof := lab.Send(Request{Port: port, Method: "GET", Path: "/last-proof"})
	if proof.Status == 200 && proof.Body == marker {
		return Finding{port, proof.Body}, true
	}
	return Finding{}, false
}

func main() {}
