package main

type Packet struct {
	Proto   string `json:"proto"`
	Src     string `json:"src"`
	Dst     string `json:"dst"`
	Sport   int    `json:"sport"`
	Dport   int    `json:"dport"`
	Flags   string `json:"flags"`
	Payload string `json:"payload"`
	Query   string `json:"query"`
}
type Evidence struct {
	Port    int
	Queries []string
}

func AnalyzeCapture(raw string) (Evidence, error) {
	return Evidence{}, nil
}

func main() {}
