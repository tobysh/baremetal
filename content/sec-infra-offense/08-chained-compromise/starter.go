package main

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
	return Finding{}, false
}

func main() {}
