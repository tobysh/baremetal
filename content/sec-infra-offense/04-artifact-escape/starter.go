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

// ---- Your part: implement EscapeArtifact as described in the task. ----

func EscapeArtifact(lab Lab, port int, root, protected string) (Finding, bool) {
	// Build one payload that starts with root+"/", climbs out with "../" and names protected.
	return Finding{}, false
}

func main() {}
