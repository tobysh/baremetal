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
type Unit struct {
	Name, RunAs, Command string
	Enabled, Running     bool
}

func ExerciseService(lab Lab, port int, adminToken, name, marker string) (Finding, bool) {
	send := func(method, path, body string) Reply {
		return lab.Send(Request{Port: port, Method: method, Path: path, Token: adminToken, Body: body})
	}
	unit := Unit{Name: name, RunAs: "root", Command: "emit " + marker, Enabled: true}
	body, _ := json.Marshal(unit)
	path := "/units/" + name
	if send("PUT", path, string(body)).Status != 201 {
		return Finding{}, false
	}
	send("POST", "/reboot", "")
	state := send("GET", path, "")
	var running Unit
	proof := send("GET", "/service-proof", "")
	confirmed := state.Status == 200 && json.Unmarshal([]byte(state.Body), &running) == nil && running.Name == name && running.RunAs == "root" && running.Command == unit.Command && running.Enabled && running.Running && proof.Status == 200 && proof.Body == marker
	send("POST", path+"/stop", "")
	send("DELETE", path, "")
	after := send("GET", path, "")
	send("POST", "/reboot", "")
	afterProof := send("GET", "/service-proof", "")
	if confirmed && after.Status == 404 && afterProof.Status == 200 && afterProof.Body == "" {
		return Finding{port, proof.Body}, true
	}
	return Finding{}, false
}

func main() {}
