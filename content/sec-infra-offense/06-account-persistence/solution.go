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
type Account struct {
	Name, Password string
	UID            int
	Hidden         bool
}

func ExerciseAccount(lab Lab, port int, adminToken string, account Account) (Finding, bool) {
	send := func(method, path, token, body string) Reply {
		return lab.Send(Request{Port: port, Method: method, Path: path, Token: token, Body: body})
	}
	body, _ := json.Marshal(account)
	if send("POST", "/accounts", adminToken, string(body)).Status != 201 {
		return Finding{}, false
	}
	all := send("GET", "/accounts/all", adminToken, "")
	visible := send("GET", "/accounts/visible", adminToken, "")
	contains := func(items []string) bool {
		for _, name := range items {
			if name == account.Name {
				return true
			}
		}
		return false
	}
	loginBody, _ := json.Marshal(map[string]string{"user": account.Name, "password": account.Password})
	session := send("POST", "/login", "", string(loginBody))
	proof := Reply{}
	if session.Status == 200 && session.Token != "" {
		proof = send("GET", "/root/proof", session.Token, "")
	}
	confirmed := all.Status == 200 && visible.Status == 200 && contains(all.Items) && !contains(visible.Items) && proof.Status == 200 && strings.HasPrefix(proof.Body, "LAB-PROOF:")
	send("DELETE", "/accounts/"+account.Name, adminToken, "")
	after := send("GET", "/accounts/all", adminToken, "")
	loginAfter := send("POST", "/login", "", string(loginBody))
	revoked := true
	if session.Status == 200 && session.Token != "" {
		r := send("GET", "/root/proof", session.Token, "")
		revoked = r.Status == 401 || r.Status == 403
	}
	if confirmed && after.Status == 200 && !contains(after.Items) && loginAfter.Status == 401 && revoked {
		return Finding{port, proof.Body}, true
	}
	return Finding{}, false
}

func main() {}
