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

// ---- Your part: implement ExploitAgent as described in the task. ----

func ExploitAgent(lab Lab, port int, marker string) (Finding, bool) {
	// 1. version gate  2. one job whose label carries a second command  3. independent read of /last-proof
	return Finding{}, false
}

func main() {}
