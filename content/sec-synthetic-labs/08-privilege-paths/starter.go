package main

type File struct {
	Path         string
	Mode         uint32
	Owner, Group string
}
type Job struct{ RunAs, Path string }
type SudoRule struct {
	User, RunAs, Path string
	NoPassword        bool
}
type State struct {
	User   string
	Groups []string
	Files  []File
	Cron   []Job
	Sudo   []SudoRule
}

func EscalationPaths(raw string) ([]string, error) {
	return nil, nil
}

func main() {}
