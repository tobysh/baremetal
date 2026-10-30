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
type Account struct {
	Name, Password string
	UID            int
	Hidden         bool
}

func ExerciseAccount(lab Lab, port int, adminToken string, account Account) (Finding, bool) {
	return Finding{}, false
}

func main() {}
