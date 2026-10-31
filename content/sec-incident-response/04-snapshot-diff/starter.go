package main

type File struct {
	Path, Hash, Owner string
	Mode              uint32
	MTime, CTime      int64
}

type Change struct {
	Path, Kind string
	Timestomp  bool
}

func Diff(base, now []File, allowed map[string]string) []Change {
	return nil
}

func main() {}
