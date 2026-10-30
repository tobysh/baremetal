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
type BackupConfig struct {
	Path, Owner, Command string
	Writable             bool
}

func ExerciseBackup(lab Lab, port int, token string) (Finding, bool) {
	send := func(method, path, body string) Reply {
		return lab.Send(Request{Port: port, Method: method, Path: path, Token: token, Body: body})
	}
	identity := send("GET", "/identity", "")
	if identity.Status != 200 || identity.Body != "user" {
		return Finding{}, false
	}
	r := send("GET", "/backup/config", "")
	var original BackupConfig
	if r.Status != 200 || json.Unmarshal([]byte(r.Body), &original) != nil || original.Owner != "root" || !original.Writable || original.Command != "archive" {
		return Finding{}, false
	}
	changed := original
	changed.Command = "grant-root"
	body, _ := json.Marshal(changed)
	if send("PUT", "/backup/config", string(body)).Status != 200 {
		return Finding{}, false
	}
	send("POST", "/backup/run", "")
	proof := send("GET", "/root/proof", "")
	body, _ = json.Marshal(original)
	send("PUT", "/backup/config", string(body))
	send("POST", "/session/drop-root", "")
	check := send("GET", "/backup/config", "")
	var restored BackupConfig
	role := send("GET", "/identity", "")
	clean := check.Status == 200 && json.Unmarshal([]byte(check.Body), &restored) == nil && restored == original && role.Status == 200 && role.Body == "user"
	if proof.Status == 200 && strings.HasPrefix(proof.Body, "LAB-PROOF:") && clean {
		return Finding{port, proof.Body}, true
	}
	return Finding{}, false
}

func main() {}
