package main

type Login struct {
	Time    int64
	User    string
	IP      string
	OK      bool
	Session string // issued session id, only on successful logins
}

type Request struct {
	Time    int64
	IP      string
	Session string
	Path    string
}

type Link struct {
	Index int
	User  string
	Basis string
}

func Attribute(logins []Login, reqs []Request, window int64) []Link {
	return nil
}

func main() {}
