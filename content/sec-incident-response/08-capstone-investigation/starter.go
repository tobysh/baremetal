package main

type File struct {
	Path, Hash, Owner string
	Mode              uint32
	MTime, CTime      int64
}

type Edge struct{ From, To string }

type Audit struct {
	ID              string
	Time            int64
	Actor, From, To string
}

type Case struct {
	Auth, Web     []string
	Base, Now     []File
	Allowed       map[string]string
	Before, After []Edge
	Audit         []Audit
}

type Finding struct {
	Kind, Subject string
	Cites         []string
}

type Report struct {
	Findings    []Finding
	Timeline    []string
	Assumptions []string
}

func Investigate(c Case) Report {
	return Report{}
}

func main() {}
