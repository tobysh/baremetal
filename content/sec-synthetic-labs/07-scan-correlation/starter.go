package main

type Advisory struct{ Service, Version, ID string }
type Exposure struct {
	Port int
	ID   string
}

func Correlate(scan string, table []Advisory) []Exposure {
	return nil
}

func main() {}
