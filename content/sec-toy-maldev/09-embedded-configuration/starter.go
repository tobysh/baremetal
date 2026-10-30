package main

import (
	"fmt"
)

type Config struct {
	Label    string
	Interval uint16
	MaxOps   uint16
	Allow    []string
	Token    string
}

func Decode(b []byte) (Config, error) { return Config{}, nil }

func Encode(c Config) ([]byte, error) { return nil, nil }

func Describe(c Config) string { return "" }

func main() {
	c := Config{Label: "lab", Interval: 10, MaxOps: 64, Allow: []string{"a", "b"}, Token: "lab-token-1"}
	b, err := Encode(c)
	fmt.Printf("% x (%v)\n", b, err)
	d, err := Decode(b)
	fmt.Println(Describe(d), err)
}
