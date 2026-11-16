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

// ---- Your part: implement GuessManagement as described in the task. ----

func GuessManagement(lab Lab, port int, user string, passwords []string) (Finding, bool) {
	// For each permitted password: login, treat a token as a candidate, then confirm with /admin/proof.
	return Finding{}, false
}

func main() {}
