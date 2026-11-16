package main

// ---- Supplied scaffolding: do not change these types. ----

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

// ---- Your part: implement DiscoverStore as described in the task. ----

func DiscoverStore(lab Lab, ports []int) (Finding, bool) {
	// For each port: banner gate, key listing, then read values until one starts with "LAB-PROOF:".
	return Finding{}, false
}

func main() {}
