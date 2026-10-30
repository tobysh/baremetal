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
type BackupConfig struct {
	Path, Owner, Command string
	Writable             bool
}

func ExerciseBackup(lab Lab, port int, token string) (Finding, bool) {
	return Finding{}, false
}

func main() {}
