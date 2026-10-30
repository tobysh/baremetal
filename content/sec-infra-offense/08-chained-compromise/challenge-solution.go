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
type LeakConfig struct {
	ManagementPort int    `json:"management_port"`
	TokenKey       string `json:"token_key"`
}
type LeakValue struct {
	Token string `json:"token"`
}

func ChainSnapshot(lab Lab, ports []int) (Finding, bool) {
	scope := map[int]bool{}
	for _, port := range ports {
		scope[port] = true
	}
	for _, port := range ports {
		banner := lab.Send(Request{Port: port, Method: "GET", Path: "/banner"})
		if banner.Status != 200 || banner.Body != "labbackup/0.8" {
			continue
		}
		r := lab.Send(Request{Port: port, Method: "GET", Path: "/public/config"})
		var cfg LeakConfig
		if r.Status != 200 || json.Unmarshal([]byte(r.Body), &cfg) != nil || cfg.TokenKey == "" || !scope[cfg.ManagementPort] {
			continue
		}
		r = lab.Send(Request{Port: port, Method: "GET", Path: "/public/value/" + cfg.TokenKey})
		var value LeakValue
		if r.Status != 200 || json.Unmarshal([]byte(r.Body), &value) != nil || value.Token == "" {
			continue
		}
		proof := lab.Send(Request{Port: cfg.ManagementPort, Method: "GET", Path: "/admin/snapshot", Token: value.Token})
		if proof.Status == 200 && strings.HasPrefix(proof.Body, "LAB-PROOF:") {
			return Finding{cfg.ManagementPort, proof.Body}, true
		}
	}
	return Finding{}, false
}

func main() {}

func ChainAndRotate(lab Lab, ports []int) (Finding, bool) {
	scope := map[int]bool{}
	for _, p := range ports {
		scope[p] = true
	}
	for _, p := range ports {
		send := func(port int, method, path, token string) Reply {
			return lab.Send(Request{Port: port, Method: method, Path: path, Token: token})
		}
		b := send(p, "GET", "/banner", "")
		if b.Status != 200 || b.Body != "labbackup/0.8" {
			continue
		}
		r := send(p, "GET", "/public/config", "")
		var cfg LeakConfig
		if r.Status != 200 || json.Unmarshal([]byte(r.Body), &cfg) != nil || cfg.TokenKey == "" || !scope[cfg.ManagementPort] {
			continue
		}
		r = send(p, "GET", "/public/value/"+cfg.TokenKey, "")
		var value LeakValue
		if r.Status != 200 || json.Unmarshal([]byte(r.Body), &value) != nil || value.Token == "" {
			continue
		}
		proof := send(cfg.ManagementPort, "GET", "/admin/snapshot", value.Token)
		if proof.Status != 200 || !strings.HasPrefix(proof.Body, "LAB-PROOF:") {
			continue
		}
		rotated := send(cfg.ManagementPort, "POST", "/admin/rotate", value.Token)
		if rotated.Status != 200 || rotated.Token == "" || rotated.Token == value.Token {
			return Finding{}, false
		}
		old := send(cfg.ManagementPort, "GET", "/admin/snapshot", value.Token)
		fresh := send(cfg.ManagementPort, "GET", "/admin/snapshot", rotated.Token)
		send(p, "DELETE", "/public/value/"+cfg.TokenKey, rotated.Token)
		after := send(p, "GET", "/public/value/"+cfg.TokenKey, "")
		if (old.Status == 401 || old.Status == 403) && fresh.Status == 200 && fresh.Body == proof.Body && after.Status == 404 {
			return Finding{cfg.ManagementPort, proof.Body}, true
		}
		return Finding{}, false
	}
	return Finding{}, false
}
